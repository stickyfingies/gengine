#include <fstream>
#include <iostream>
#include <sstream>

#include "duktape.h"
#include "dukglue/dukglue.h"

using namespace std;

void foo(const char* text) {
    cout << text << endl;
}

int main(int argc, char* argv[])
{
	const char* script_path = "./typescript/script.js";

	ifstream script_file(script_path);
	if (!script_file.is_open()) {
		cerr << "Failed to open " << script_path << endl;
	}
	stringstream script_data;
	script_data << script_file.rdbuf();
	string script_str = script_data.str();

    /* === */

	duk_context* ctx = duk_create_heap_default();

    dukglue_register_function(ctx, foo, "print");

	duk_eval_string(ctx, script_str.data());

	duk_destroy_heap(ctx);

	return 0;
}