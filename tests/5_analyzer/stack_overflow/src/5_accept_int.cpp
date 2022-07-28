#include <cstdlib>
#include <random>

int volatile i;

void so(int p) {
	so(p);
    i = p; // prevent stack using optimizing
}

int main() {
	void * p = new char(42);
	srand((unsigned long long) p);
	alloca(rand() % 64 + 1); // initial padding on stack
	so(42);
}
