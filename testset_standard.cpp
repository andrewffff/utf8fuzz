
#include "testset_standard.h"

bool TestSetStandard::next(std::vector<unsigned char>& dest) {
	if(currentTest != tests.end())
		++currentTest;

	if(currentTest == tests.end())
		return false;

	dest = currentTest->data;
	return true;
}


std::string TestSetStandard::description() {
	return currentTest->name;
}


TestSetStandard::TestSetStandard() {
	std::vector<unsigned char> v;

	v.clear();
	tests.push_back(OneTest("you need to call next()", v));

	v.clear();
	v.push_back('H');
	v.push_back('e');
	v.push_back('l');
	v.push_back('l');
	v.push_back('o');
	tests.push_back(OneTest("Hello", v));

	v.clear();
	v.push_back('x');
	v.push_back('\0');
	tests.push_back(OneTest("Letter followed by 0 byte", v));

	v.clear(); // crêpe
	v.push_back(0x63);
	v.push_back(0x72);
	v.push_back(0xC3);
	v.push_back(0xAA);
	v.push_back(0x70);
	v.push_back(0x65);
	tests.push_back(OneTest("crepe with a ^ over the middle character", v));

	v.clear(); // crêpe
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	tests.push_back(OneTest("3 x crepe with a ^ over the middle character", v));

	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	v.push_back(0x63); v.push_back(0x72); v.push_back(0xC3); v.push_back(0xAA); v.push_back(0x70); v.push_back(0x65);
	tests.push_back(OneTest("7 x crepe with a ^ over the middle character", v));

	v.clear();
	v.push_back(0xe3);
	v.push_back(0x81);
	v.push_back(0x8c);
	tests.push_back(OneTest("a japanese character", v));

	v.clear(); 
	v.push_back(0xf4);
	v.push_back(0x8f);
	v.push_back(0xbf);
	v.push_back(0xbf);
	tests.push_back(OneTest("largest character allowed under the RFC", v));

	v.clear(); 
	v.push_back(0xf4);
	v.push_back(0x90);
	v.push_back(0x80);
	v.push_back(0x80);
	tests.push_back(OneTest("largest character allowed under the RFC, plus one", v));

	v.clear(); 
	v.push_back(0xC0);
	v.push_back(0x80);
	tests.push_back(OneTest("oversize null byte ('modified UTF-8')", v));

	v.clear(); 
	v.push_back(0xE0);
	v.push_back(0x83);
	v.push_back(0xBF);
	tests.push_back(OneTest("char 255 encoded as 3 characters (it should be two)", v));

	v.clear();
	v.push_back(0xED); v.push_back(0xA0); v.push_back(0x81);
	v.push_back(0xED); v.push_back(0xB0); v.push_back(0x80);
	tests.push_back(OneTest("UTF-16 surrogates incorrectly encoded, example from http://en.wikipedia.org/wiki/CESU-8", v));

	v.clear();
	v.push_back(0xF0); v.push_back(0x90); v.push_back(0x90); v.push_back(0x80);
	tests.push_back(OneTest("Previous CESU-8 example correctly coded as UTF-8", v));

	v.clear();
	v.push_back(0xEF); v.push_back(0xBB); v.push_back(0xBF);
	tests.push_back(OneTest("BOM character - see http://en.wikipedia.org/wiki/UTF-8#Byte_order_mark", v));

	v.clear();
	tests.push_back(OneTest("Empty string", v));

	v.clear();
	v.push_back(0xFE); v.push_back(0xFE); v.push_back(0xFE); v.push_back(0xFE);
	tests.push_back(OneTest("Memory scribble pattern, 0xFEFEFEFE", v));

	// include most of the kuhn tests
#include "kuhn_extracted.h"
	
	// Stuff below is regression tests (ie was once broken :) from the vectored codec implementation
	v.clear();
	v.push_back(0xE1); v.push_back(0x85); v.push_back(0xA7); v.push_back(0xCE); v.push_back(0x8C);
	tests.push_back(OneTest("Regression - E1.85.A7.CE.8C", v));

	// There was a bug with propagating requirements to the next vector
	v.clear();
	v.push_back(0xe0); v.push_back(0xa8); v.push_back(0x86); v.push_back(0xd6);
	v.push_back(0xb0); v.push_back(0xc7); v.push_back(0x9b); v.push_back(0xd8);
	v.push_back(0xb2); v.push_back(0xe0); v.push_back(0xa7); v.push_back(0xbf);
	v.push_back(0x1d); v.push_back(0x3e);
	v.push_back(0xE1); v.push_back(0x85); v.push_back(0xA7); v.push_back(0xCE); v.push_back(0x8C);
	tests.push_back(OneTest("Regression - offset 14, E1.85.A7.CE.8C", v)); 

	currentTest = tests.begin();
}

