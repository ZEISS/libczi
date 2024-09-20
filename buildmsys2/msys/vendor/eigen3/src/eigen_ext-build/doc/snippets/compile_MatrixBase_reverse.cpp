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
MatrixXi m = MatrixXi::Random(3,4);
cout << "Here is the matrix m:" << endl << m << endl;
cout << "Here is the reverse of m:" << endl << m.reverse() << endl;
cout << "Here is the coefficient (1,0) in the reverse of m:" << endl
     << m.reverse()(1,0) << endl;
cout << "Let us overwrite this coefficient with the value 4." << endl;
m.reverse()(1,0) = 4;
cout << "Now the matrix m is:" << endl << m << endl;

}
  return 0;
}
