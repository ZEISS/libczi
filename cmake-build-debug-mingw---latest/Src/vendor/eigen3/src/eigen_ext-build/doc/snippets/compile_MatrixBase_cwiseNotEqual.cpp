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
MatrixXi m(2,2);
m << 1, 0,
     1, 1;
cout << "Comparing m with identity matrix:" << endl;
cout << m.cwiseNotEqual(MatrixXi::Identity(2,2)) << endl;
Index count = m.cwiseNotEqual(MatrixXi::Identity(2,2)).count();
cout << "Number of coefficients that are not equal: " << count << endl;

}
  return 0;
}
