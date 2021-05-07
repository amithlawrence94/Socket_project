#include <iostream>
#include <thread>
#include <string>

using namespace std::literals::chrono_literals;
using namespace std;
bool finished = false;

void copyCommand(){
  string str;
  getline(cin, str);
  cout << str << endl;
  finished = true;

}

int main(){
  thread worker(copyCommand);
  while(!finished){
    cout << "Hello" << endl;
    this_thread::sleep_for(1s);
  }
  worker.detach();
  return 0;

}
