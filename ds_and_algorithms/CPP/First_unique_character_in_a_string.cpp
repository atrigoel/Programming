/*------------------------------------
PROBLEM STATEMENT:
--------------------------------------

Given a string s, find the first non-repeating character in it and return its index. If it does not exist, return -1.

Constraints:
1 <= s.length <= 10^5
s consists of only lowercase English letters.

Example 1:
Input: s = "codingminutes"
Output: 0

Example 2:
Input: s = "aabb"
Output: -1
*/

#include<iostream>
#include<string>

using namespace std;

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <string>" << endl;
        return 1;
    }
    string s = argv[1];
    int count[26] = {0};
    for (char c : s) {
        count[c - 'a']++;
    }
    for (int i = 0; i < s.length(); i++) {
        if (count[s[i] - 'a'] == 1) {
            cout << i << endl;
            return 0;
        }
    }
    cout << -1 << endl;
    return 0;
}