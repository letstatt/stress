#include <cstdlib>
#include <random>
#include <thread>

volatile bool b;

void so() {
	so();
}

int main() {
	void * p = new char(42);
	srand((unsigned long long) p);
	alloca(rand() % 64 + 1); // initial padding on stack
	std::thread t(&so);
	t.join();
}
