#include <iostream>
#include "../utils/json.hpp"

using namespace std;
using json = nlohmann::json;

// long long ╦у╨ц©ирт

int main()
{
    auto jsonText = "{ \"val\" : 4294967296 }";
    auto json = json::parse(jsonText);
    auto val = json["val"].get<long long>();
    auto i = json["val"].get<int>();
    cout << "ll " << val << endl;
    cout << "int" << i << endl;
    bool longOverflow = json["val"] != val;
    bool intOverflow = json["val"] != i;
    std::cout << std::boolalpha << "Long: " << longOverflow << "\nInt: " << intOverflow;
}