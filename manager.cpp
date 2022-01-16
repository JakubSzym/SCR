#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include <deque>
#include <algorithm>
#include <vector>
#include <mutex>
#include <condition_variable>

using namespace std;

enum Strategy
{
  FCFS = 0,     // First Come First Served
  SJF  = 1,     // Shortest Jobtime First
  HPF  = 2      // Highest Priority First
};

Strategy strategy;

struct Process
{
  int pid_;
  int priority_;
  int timeval_;

  bool operator < (Process const &other)
  {
    if (strategy == SJF)
    {
      return timeval_ < other.timeval_; 
    }
    else if (strategy == HPF)
    {
      return priority_ > other.priority_;
    }
    return false;
  }

};

deque<Process> tasks;

struct Core
{
  Process process_;
  bool busy_;
  char nothingToDo_;
  int counter_;
};

mutex mtx;
condition_variable condvar;

bool readingDone  = false;
bool nextLineRead = false;

bool allCoresFree(Core *cores, int coresNumber)
{
  for (int i = 0; i < coresNumber; i++)
  {
    if (cores[i].busy_ == true)
    {
      return false;
    }
  }

  return true;
}

void manage(int coresNumber)
{
  Core *cores = new Core[coresNumber];
  int timepoint = 0;

  for (int i = 0; i < coresNumber; i++)
  {
    cores[i].busy_ = false;
    cores[i].nothingToDo_ = '-';
    cores[i].counter_ = 0;
  }

  cout << timepoint++ << " | ";
  while (!(readingDone &&
           tasks.empty() && 
           allCoresFree(cores, coresNumber)))
  {
    unique_lock<mutex> locker(mtx);
    condvar.wait(locker, [](){ return nextLineRead; });

    if (!allCoresFree(cores, coresNumber))
    {
      cout << timepoint++ << " | ";
    }

	  for (int i = 0; i < coresNumber; i++)
	  {
	    if (cores[i].busy_ == true &&
		      cores[i].counter_ == cores[i].process_.timeval_)
		  {
		    cores[i].busy_ = false;
		    cores[i].counter_ = 0;
      }
      
      if (strategy == SJF || strategy == HPF)
      {
        sort(tasks.begin(), tasks.end());
      }

	    if (cores[i].busy_ == false && !tasks.empty())
		  {
        cores[i].process_ = tasks.front();
        tasks.pop_front();
		    cores[i].busy_ = true;
		  }
    }

    if (!allCoresFree(cores, coresNumber))
    {
      for (int i = 0; i < coresNumber; i++)
      {
        if (cores[i].busy_ == true)
        {
          cout << cores[i].process_.pid_ << " ";
          cores[i].counter_++;
        }
        else
        {
          cout << cores[i].nothingToDo_ << " ";
        }
      }
    }

    if (!allCoresFree(cores, coresNumber))
    {
     cout << endl;
    }

    locker.unlock();
    condvar.notify_one();
  }
  delete[] cores;
}

void read()
{
  for (string line; getline(cin, line); )
  {
    nextLineRead = false;
    vector<int> numbers;
    stringstream stream(line);
    string word;

    while (getline(stream, word, ' '))
    {
	    numbers.push_back(stoi(word));
    }
    
    if ((numbers.size() - 1) % 3)
	  {
	    cout << "ERROR: Wrong data format" << endl;
	    exit(1);
	  }
      
    int count = (numbers.size() - 1) / 3;

    if (count != 0)
	  {
	    for (int i = 0; i < count; i++)
	    {
	      Process p =
		    {
		      .pid_      = numbers[3 * i + 1],
		      .priority_ = numbers[3 * i + 2],
		      .timeval_  = numbers[3 * i + 3]
        };
        tasks.push_back(p);
	    }
    }

    nextLineRead = true;
    condvar.notify_one();
  }
  readingDone = true;
}

int main(int argc, char **argv)
{
  int code, coresNumber, timeQuant;

  if (argc < 2)
  {
    cout << "ERROR: Too few arguments" << endl;
    exit(1);
  }
  else if (argc == 2)
  {
    code        = stoi(argv[1]);
    coresNumber = 1;
    timeQuant   = 1;
  }
  else if (argc == 3)
  {
    code        = stoi(argv[1]);
    coresNumber = stoi(argv[2]);
    timeQuant   = 1;
  }
  else
  {
    code        = stoi(argv[1]);
    coresNumber = stoi(argv[2]);
    timeQuant   = stoi(argv[3]);
  }

  switch (code)
  {
  case 0:
    strategy = FCFS;
    break;
  case 1:
    strategy = SJF;
    break;
  case 2:
    strategy = HPF;
    break;
  default:
    cout << "ERROR: Invalid strategy code. " << endl;
    exit(1);
    break;
  }

  thread t_read(read);
  thread t_manage(manage, coresNumber);

  t_read.join();
  t_manage.join();

  cout << endl;
}
