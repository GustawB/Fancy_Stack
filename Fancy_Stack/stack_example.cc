#include "stack.h"
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

using std::swap;
using std::invalid_argument;
using std::pair;
using std::vector;
using cxx::stack;

int main() {
    stack<int, int> stack1;
    stack1.push(3, 10);
    stack1.push(1, 11);
    stack1.push(2, 12);

    int& ref1 = stack1.front().second;

    stack<int, int> stack2(stack1); // Wykonuje siÄ peĹna kopia.
    stack<int, int> stack3;
    stack3 = stack2;                // Nie wykonuje siÄ kopia.

    assert(stack1.front().first == 2);
    assert(stack1.front().second == 12);
    ref1 = 20;
    assert(stack1.front().first == 2);
    assert(stack1.front().second == 20);

    // Wymuszamy wywoĹanie metod front w wersji const.
    assert((static_cast<const stack<int, int>>(stack2)).front().first == 2);
    assert((static_cast<const stack<int, int>>(stack2)).front().second == 12);
    assert((static_cast<const stack<int, int>>(stack3)).front().first == 2);
    assert((static_cast<const stack<int, int>>(stack3)).front().second == 12);

    stack2.pop(); // Obiekt stack2 dokonuje kopii i przestaje wspĂłĹdzieliÄ dane z obiektem stack3.
    assert(stack2.size() == 2);
    assert(stack2.count(1) == 1);
    assert(stack2.count(2) == 0);
    assert(stack2.count(3) == 1);
    assert(stack3.size() == 3);
    assert(stack3.count(1) == 1);
    assert(stack3.count(2) == 1);
    assert(stack3.count(3) == 1);

    stack3.pop(1);
    assert(stack3.size() == 2);
    assert(stack3.count(1) == 0);
    assert(stack3.count(2) == 1);
    assert(stack3.count(3) == 1);

    stack1.clear();
    assert(stack1.size() == 0);
    assert(stack1.count(3) == 0);

    bool catched = false;
    try {
        stack1.pop();
        assert(0);
    }
    catch (invalid_argument&) {
        catched = true;
    }
    catch (...) {
        assert(0);
    }
    assert(catched);

    stack1.push(7, 1);
    stack1.push(7, 2);
    stack1.push(8, 3);
    stack1.push(8, 4);
    int k = 7;
    for (stack<int, int>::const_iterator it = stack1.cbegin(); it != stack1.cend(); ++it)
        assert(*it == k++);

    int& ref2 = stack1.front(7);
    assert(stack1.front(7) == 2);
    ref2 = 33;
    assert(stack1.front(7) == 33);
    stack1.pop(7);
    assert(stack1.front(7) == 1);

    // Wymuszamy wywoĹanie metod front w wersji const.
    stack<int, int> const stack4 = stack2;
    pair<int const&, int const&> p = stack4.front();
    int const& r = stack4.front(3);
    assert(p.first == 1 && p.second == 11 && r == 10);

    swap(stack1, stack3); // DziaĹa w czasie staĹym.

    vector<stack<int, int>> vec;
    for (int i = 0; i < 100000; i++)
        stack1.push(i, i);
    for (int i = 0; i < 1000000; i++)
        vec.push_back(stack1);  // Wszystkie obiekty w vec wspĂłĹdzielÄ dane.
}