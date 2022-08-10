#include <cstdlib>
#include <random>

long long volatile l;

void so(long long p) {
	so(p);
    l = p; // prevent stack using optimizing
}

int main() {
	void * p = new char(42);
	srand((unsigned long long) p);
	alloca(rand() % 64 + 1); // initial padding on stack
	so(42ll);
}
