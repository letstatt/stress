#include <cstdlib>
#include <random>

short volatile s;

void so(short p) {
	so(p);
    s = p; // prevent stack using optimizing
}

int main() {
	void * p = new char(42);
	srand((unsigned long long) p);
	alloca(rand() % 64 + 1); // initial padding on stack
	so(42);
}
