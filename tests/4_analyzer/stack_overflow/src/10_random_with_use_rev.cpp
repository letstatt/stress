#include <cstdlib>
#include <random>

volatile bool b;

void so() {
	int pad = rand() % 64 + 1;
	void* ptr = alloca(pad);
	for (int i = 0; i < pad / sizeof(bool); --i) {
		b = ((bool*) ptr)[i];
	}
	so();
}

int main() {
	void * p = new char(42);
	srand((unsigned long long) p);
	alloca(rand() % 64 + 1); // initial padding on stack
	so();
}
