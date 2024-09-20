static bool eigen_did_assert = false;
#define eigen_assert(X) if(!eigen_did_assert && !(X)){ std::cout << "### Assertion raised in " << __FILE__ << ":" << __LINE__ << ":\n" #X << "\n### The following would happen without assertions:\n"; eigen_did_assert = true;}

#include <iostream>
#include <Eigen/Eigen>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif


using namespace Eigen;
using namespace std;

int main(int, char**)
{
  cout.precision(3);
// intentionally remove indentation of snippet
{
cout << "Even spacing inputs:" << endl;
cout << VectorXi::LinSpaced(8,1,4).transpose() << endl;
cout << VectorXi::LinSpaced(8,1,8).transpose() << endl;
cout << VectorXi::LinSpaced(8,1,15).transpose() << endl;
cout << "Uneven spacing inputs:" << endl;
cout << VectorXi::LinSpaced(8,1,7).transpose() << endl;
cout << VectorXi::LinSpaced(8,1,9).transpose() << endl;
cout << VectorXi::LinSpaced(8,1,16).transpose() << endl;

}
  return 0;
}
