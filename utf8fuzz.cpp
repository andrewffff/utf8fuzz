
#include <unistd.h>

#include <vector>
#include <string>
#include <cstdio>
#include <cassert>
#include <cerrno>
#include <memory>

#include "verbosity.h"

#include "aligned_alloc.h"

#include "codecs/AfUtf8/memory.h"

#include "testset_standard.h"
#include "testset_random.h"
#include "testset_files.h"


#ifdef __APPLE__
// we don't use autoconf, so give cycle.h what it needs
#define HAVE_MACH_ABSOLUTE_TIME
#define HAVE_MACH_MACH_TIME_H
#endif

#include "contrib/cycle.h"



using namespace std;
using AfUtf8::memory;




#define DEFAULT_RANDOM_COUNT (300)


#include "validator.h"



class Runner {
private:
	bool dieOnFirstFailure;
	bool testAlignment;
	bool forceAlignment, forceAlignmentFromEnd;
	int forceAlignmentOfs;
	int verbosity, repeatCount, printTimings;
	vector<Validator*> validators;

	int totals[3];
	int goodAccept, goodReject;

	struct TimingResult {
		Validator* validator;
		string itemName;
		size_t itemLength;
		vector<double> timings;

		TimingResult(Validator* v, string n, int l, vector<double> t) : validator(v), itemName(n), itemLength(l), timings(t) {;}
	};

	list<TimingResult> timingResults;

	static const int RESULT_GOOD = 0;
	static const int RESULT_CONFUSION = 1;
	static const int RESULT_FAILURE = 2;

public:
	Runner(bool dieOnFirstFailure, vector<Validator*> v, int verb, bool testAlignment, bool fa, bool fafe, int fao, int repeat, bool pt)
		: dieOnFirstFailure(dieOnFirstFailure), testAlignment(testAlignment),
			forceAlignment(fa), forceAlignmentFromEnd(fafe), forceAlignmentOfs(fao),
			verbosity(verb), repeatCount(repeat), printTimings(pt), validators(v),
			totals({0, 0, 0}), goodAccept(0), goodReject(0)
   	{
		assert(repeatCount > 0);
	}

	~Runner() {
		for(Validator* p : validators)
			delete p;
	}

	/** returns RESULT_xxx after applying that result to our totals, and logging timing information */
	int run(const string& description, const memory<unsigned char>& v) {
		//printf("START: %p\nEND: %p\n", v.begin(), v.end());

		int we_accept = 0, other_accept = 0, we_reject = 0, other_reject = 0;

		// if debug output is coming, print header
		bool headerPrinted = (verbosity >= 3);
		if(headerPrinted)
			printf("\n\n###\n%s\n", description.c_str());

		// run everything and accumulate results
		vector<bool> results;
		for(Validator* p : validators) {
			vector<double> timings(repeatCount);

			bool r; // result
			for(int i = 0; i < repeatCount; ++i) {
				p->prepare(v.size());
				ticks t0 = getticks();
				bool thisResult = p->validate(v);
				ticks t1 = getticks();

				timings[i] = elapsed(t1, t0);

				// safety - bomb out if we get different results from different runs
				if(i == 0) {
					results.push_back(thisResult);
					r = thisResult;
				} else if(r != thisResult) {
					printf("ERROR: inconsistent results from runs by same validator against same data\n");
					exit(1);
				}
			}

			timingResults.push_back(TimingResult(p, description, v.size(), timings));

			if(r) {
				if(p->ours())
					we_accept++;
				else
					other_accept++;
			} else {
				if(p->ours())
					we_reject++;
				else
					other_reject++;
			}
		}

		// what's the overall result? apply to total.
		int result;

		if(we_accept + other_accept == 0) {
			result = RESULT_GOOD;
			goodReject++;
		} else if(we_reject + other_reject == 0) {
			result = RESULT_GOOD;
			goodAccept++;
		} else if(other_accept != 0 && other_reject != 0) {
			result = RESULT_CONFUSION;
		} else {
			result = RESULT_FAILURE;
		}

		totals[result]++;


		// print if necessary (due to error and/or verbosity level)
		if(verbosity >= 2 || result != RESULT_GOOD) {
			if(!headerPrinted)
				printf("%s -- ", description.c_str());

			vector<bool>::const_iterator resultp = results.begin();
			for(const Validator* p : validators) {
				printf("%s %s", p->name().c_str(), *resultp ? "accept, " : "reject, ");
				++resultp;
			}

			switch(result) {
				case RESULT_GOOD: printf("good.\n"); break;
				case RESULT_CONFUSION: printf("CONFUSION!\n"); break;
				case RESULT_FAILURE: printf("FAILURE!\n"); break;
			}
		}

		return result;
	}

	void runAll(TestSet& tests) {
		vector<double> times;
		times.reserve(repeatCount);

		vector<unsigned char> v;
		while(tests.next(v)) {
			// give a particular alignment only if necessary
			unique_ptr<my_aligned_alloc> aa;
			memory<unsigned char> m(v);
			if(testAlignment) {
				bool alignEnd = tests.alignEnd();
				int alignOfs = tests.alignment();
				if(forceAlignment) {
					alignEnd = forceAlignmentFromEnd;
					alignOfs = forceAlignmentOfs;
				}

				// aligned_alloc doesn't do empty allocations, so
				// put those on the zero page
				if(!m.size()) {
					intptr_t ptr = alignEnd ? (64 - alignOfs) : alignOfs;
					m = memory<unsigned char>((unsigned char*) ptr, (unsigned char*) ptr);
				} else {
					aa = unique_ptr<my_aligned_alloc>(new my_aligned_alloc(
								v,
								alignEnd ? my_aligned_alloc::END_OF_PAGE : my_aligned_alloc::START_OF_PAGE,
								alignOfs));
					m = memory<unsigned char>(aa.get()->begin(), aa.get()->end());
				}
			}

			if(RESULT_GOOD != run(tests.description(), m) && dieOnFirstFailure) {
				printf("\n\nStopping early due to error\n");
				break;
			}
		}

		printf("\n\n");
		printf("Good: %0d   (%0d good utf8, %0d bad utf8),    confused: %0d,   failed: %0d\n",
				totals[RESULT_GOOD], goodAccept, goodReject, totals[RESULT_CONFUSION], totals[RESULT_FAILURE]);

		if(printTimings) {
			for(TimingResult& r: timingResults) {
				printf("%s,\"%s\",%lld", r.validator->name().c_str(), r.itemName.c_str(), (long long) r.itemLength);
				for(double d : r.timings)
					printf(",%g", d);
				printf("\n");
			}
		}
	}
};


const char *ARGV0 = "<path-to-program>";

void usage() {
	printf("USAGE: %s [<options>] [<filenames...>]\n", ARGV0);
	printf("\n");
	printf("Options:\n");
	printf("  -k   Continue to run tests even after an error is encountered.\n");
	printf("  -b   Include the C library mbs* function and GNU iconv validators.\n");
	printf("       They accept not-quite-valid UTF-8 that the rest of the codecs\n");
	printf("       reject. So use with -k!\n");
	printf("  -c x Use only one codec. See possibilities below.\n");
	printf("  -m   Vary memory alignment for testing. Consider using MallocGuardEdges=1.\n");
	printf("  -M a Consistent memory alignment of <a> bytes for testing. Prefix a with\n");
	printf("       a '-' to align to the end of a page (eg '-0' to be flush up against\n");
	printf("       the end.\n");
#ifdef BENCHMARK_BUILD
	printf("  -v   Verbosity - WILL NOT WORK as this was build with BENCHMARK_BUILD.\n");
#else
	printf("  -v   Verbose (-vv, -vvv etc for more verbose).\n");
#endif
	printf("  -s   Run some hardcoded tests.\n");
	printf("  -r   Randomly generate test data (valid and invalid UTF-8) - %0d samples.\n", DEFAULT_RANDOM_COUNT);
	printf("  -R n As with -r, but generate n samples.\n");
	printf("  -S n Use n as initial random seed for all randomness (default = 1).\n");
	printf("  -V   Randomly generate only valid UTF-8.\n");
	printf("  -l m Minimum number of codepoints to include in a randomly generated sample.\n");
	printf("  -L m Maximum number of codepoints to include in a randomly generated sample.\n");
	printf("  -t   Run benchmarks, print CSV when finished.\n");
	printf("  -h   Display this help.\n");
	printf("\n");
	printf("If you supply filenames, then those files will be read in and each\n");
	printf("processed individually. If you do not use -s or supply filenames then\n");
	printf("test data will be randomly generated.\n");
	printf("\n");
	printf("Included validators (specify with -c):\n");
	printf("   ");

	vector<Validator*> validators = Validator::createAll(true); // XXX leak
	for(Validator* v : validators) 
		printf("%s ", v->name().c_str());
	printf("\n\n");
}


int main(int argc, char** argv) {
	bool dieOnError = true;
	bool includeBrokenImpls = false;
	bool testAlignment = false;
	bool forceAlignment = false;
	int forceAlignmentFromEnd = false;
	int forceAlignmentOfs = -1;
	bool runStandard = false;
	bool runRandom = false;
	bool runBenchmarks = false;
	int randomCount = -1;
	int verbosity = 0;
	int seed = -1;
	string selectedCodec;
	vector<string> filenames;
	int ch;
	bool entirelyValid = false;
	ssize_t minLength = -1, maxLength = -1;

	ARGV0 = strdup(argv[0]);

	// parse command line options, possibly printing usage information
	// and exiting
	while((ch = getopt(argc, argv, "kbc:mM:vrR:S:Vl:L:sht")) != -1) {
		switch(ch) {
		case 'k':
			dieOnError = false;
			break;

		case 'b':
			includeBrokenImpls = true;
			break;

		case 'c':
			selectedCodec = optarg;
			break;

		case 'm':
			testAlignment = true;
			break;

		case 'M':
			if(forceAlignment) {
				printf("ERROR: you can only specify -M once!\n");
				exit(1);
			}
			forceAlignment = true;
			testAlignment = true;
			if(*optarg == '-' || *optarg == '+') {
				forceAlignmentFromEnd = (*optarg == '-');
				forceAlignmentOfs = strtol(optarg+1, NULL, 10);
			} else {
				forceAlignmentOfs = strtol(optarg, NULL, 10);
			}
			if(forceAlignmentOfs < 0 || forceAlignmentOfs > 63) {
				printf("ERROR: \"%s\" is not a valid alignment.\n", optarg);
				exit(1);
			}
			break;

		case 'v':
			verbosity++;
			break;

		case 'r':
			runRandom = true;
			break;

		case 'R':
			runRandom = true;
			if(randomCount >= 0) {
				printf("ERROR: Cannot use -R more than once.\n");
				exit(1);
			}
			randomCount = strtol(optarg, NULL, 10);
			if(randomCount < 1) {
				printf("ERROR: \"%s\" is not a valid number of samples.\n", optarg);
				exit(1);
			}
			break;

		case 'S':
			runRandom = true;
			if(seed >= 0) {
				printf("ERROR: can't use -S more than once.\n");
				exit(1);
			}
			seed = strtol(optarg, NULL, 10);
			if(seed < 0) {
				printf("ERROR: \"%s\" is not a valid random seed.\n", optarg);
				exit(1);
			}
			break;

		case 'l':
			runRandom = true;
			if(minLength >= 0) {
				printf("ERROR: can't use -l more than once.\n");
				exit(1);
			}
			minLength = strtoll(optarg, NULL, 10);
			if(minLength < 0) {
				printf("ERROR: \"%s\" is not a valid value for -l.\n", optarg);
				exit(1);
			}
			break;

		case 'L':
			runRandom = true;
			if(maxLength >= 0) {
				printf("ERROR: can't use -L more than once.\n");
				exit(1);
			}
			maxLength = strtol(optarg, NULL, 10);
			if(maxLength < 0) {
				printf("ERROR: \"%s\" is not a valid value for -L.\n", optarg);
				exit(1);
			}
			break;

		case 'V':
			runRandom = true;
			entirelyValid = true;
			break;

		case 's':
			runStandard = true;
			break;

		case 't':
			runBenchmarks = true;
			break;

		case ':':
		case '?':
		default:
			if(ch != 'h') {
				printf("ERROR: Unrecognised argument.\n\n");
				usage();
				exit(1);
			}
			usage();
			exit(0);
		}
	}

	// remaining arguments are filenames
	for(int i = optind; i < argc; ++i)
		filenames.push_back(string(argv[i]));

	// default values
	if(randomCount < 1) randomCount = DEFAULT_RANDOM_COUNT;
	if(seed < 0) seed = 1;
	if(minLength < 0) minLength = 1;
	if(maxLength < 0) maxLength = 512*1024;

	// make sure we don't have contradictory instructions
	int invocationCount = 0;
	if(runRandom) invocationCount++;
	if(runStandard) invocationCount++;
	if(!filenames.empty()) invocationCount++;
	if(invocationCount != 1) {
		printf("ERROR: You must supply exactly one of -r, -s, (or options which imply them), or a list of files.\n\n");
		usage();
		exit(1);
	}

	if(minLength > maxLength) {
		printf("ERROR: minimum length is longer than maximum length.\n\n");
		usage();
		exit(1);
	}

#ifdef BENCHMARK_BUILD
	if(verbosity) {
		printf("ERROR: Cannot use verbosity options on a benchmark build!\n");
		exit(1);
	}
#else
	g_verbosity = verbosity;
#endif

	// Select appropriate validators
	vector<Validator*> validators = Validator::createAll(includeBrokenImpls || !selectedCodec.empty());
	if(!selectedCodec.empty()) {
		vector<Validator*> selected;
		copy_if(validators.begin(), validators.end(), back_inserter(selected),
				[selectedCodec](const Validator* v) { return v->name() == selectedCodec; });
		selected.swap(validators); // XXX leak.. meh

		if(validators.empty()) {
			printf("ERROR: Could not locate validator \"%s\".\n\n", selectedCodec.c_str());
			exit(1);
		}
	}

	shared_ptr<Runner> r(new Runner(dieOnError, validators, verbosity, testAlignment, forceAlignment, forceAlignmentFromEnd, forceAlignmentOfs, runBenchmarks ? 10 : 1, runBenchmarks));

	shared_ptr<TestSet> tests(
		!filenames.empty()
			? new TestSetFiles(filenames)
			: (runRandom
				? static_cast<TestSet*>(new TestSetRandom(seed,randomCount, minLength, maxLength, entirelyValid))
				: static_cast<TestSet*>(new TestSetStandard())));

	r->runAll(*tests);
	return 0;
}




