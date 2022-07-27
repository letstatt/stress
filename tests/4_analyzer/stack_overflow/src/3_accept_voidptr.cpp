#include <cstdlib>
#include <random>

void * volatile ptr;

void so(void* p) {
	so(p);
    ptr = p; // prevent stack using optimizing
}

int main() {
	void * p = new char(42);
	srand((unsigned long long) p);
	alloca(rand() % 64 + 1); // initial padding on stack
	so(p);
}
