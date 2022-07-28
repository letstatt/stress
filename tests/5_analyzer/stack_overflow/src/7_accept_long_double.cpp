#include <cstdlib>
#include <random>

long double volatile ld;

void so(long double p) {
	so(p);
    ld = p; // prevent stack using optimizing
}

int main() {
	void * p = new char(42);
	srand((unsigned long long) p);
	alloca(rand() % 64 + 1); // initial padding on stack
	so(42.0l);
}
