Readme for a.cpp search engine

1. use the following command to compile the code in ubuntu-

g++ a.cpp -o a -std=c++11 -fopenmp -I/home/<user>/boostcpp/include/boost/ -I /home/<user>/boostcpp/include/

2. Since this search engine is supporting dynamic corpus, any corpus can be added anytime. An assumption is made that mathematical equations, languages other than english & special unicode characters are not present in the corpus.

3. Another assumption is made that a file woll be added only once to the corpus, and after addition the file will not change.

4. github repository - https://github.com/rachitkl/seek_atheism

5. Libraries are not added in this repository. (You can always request it as and when required).
