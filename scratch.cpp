#include <memory>
#include <print>

int main() {
    std::unique_ptr<int> p = std::make_unique<int>(42);
    std::unique_ptr<const int> cp = std::move(p);
}
