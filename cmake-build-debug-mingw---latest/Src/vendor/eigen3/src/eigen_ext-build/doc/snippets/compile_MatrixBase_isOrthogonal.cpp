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
Vector3d v(1,0,0);
Vector3d w(1e-4,0,1);
cout << "Here's the vector v:" << endl << v << endl;
cout << "Here's the vector w:" << endl << w << endl;
cout << "v.isOrthogonal(w) returns: " << v.isOrthogonal(w) << endl;
cout << "v.isOrthogonal(w,1e-3) returns: " << v.isOrthogonal(w,1e-3) << endl;

}
  return 0;
}
