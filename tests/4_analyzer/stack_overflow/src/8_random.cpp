#include <cstdlib>
#include <random>

int so() {
	alloca(rand() % 64 + 1);
	so();
	return 42;
}

int main() {
	void * p = new char(42);
	srand((unsigned long long) p);
	alloca(rand() % 64 + 1); // initial padding on stack
	return so();
}
