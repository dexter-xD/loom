#include <iostream>
#include <vector>

using namespace std;
int main() {
  vector<int> nums = {1, 2, 3, 4, 5};
  for (int i = 0; i < nums.size(); i++) {
    cout << nums[i] << " ";
  }
  cout << endl;
  if (nums.size() > 0 && nums[0] == 1) {
    cout << "First element is 1" << endl;
  }
  return 0;
}