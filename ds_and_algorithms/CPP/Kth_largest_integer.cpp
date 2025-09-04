/*----------------------
PROBLEM STATEMENT:
------------------------
Given an integer array nums and an integer k, return the kth largest element in the array.
Note: that it is the kth largest element in the sorted order, not the kth distinct element.

Constraints:
1 <= k <= nums.length <= 10^4
-10^4 <= nums[i] <= 10^4

Example:
Input: nums = [3,2,3,1,2,4,5,5,6], k = 4 
Output: 4
*/

#include<algorithm>
#include<iostream>
#include<vector>

using namespace std;

int main(int argc, char const *argv[])
{
    vector<int> arr {3,2,3,1,2,4,5,5,6};
    int k = 4;
    sort(arr.begin(), arr.end(), [](int a, int b){return a>b;});
    cout << arr[k-1] << endl;
    return 0;
}
