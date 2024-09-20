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
Matrix3i m = Matrix3i::Random();
cout << "Here is the initial matrix m:" << endl << m << endl;
int i = -1;
for(auto c: m.colwise()) {
  c *= i;
  ++i;
}
cout << "Here is the matrix m after the for-range-loop:" << endl << m << endl;
auto cols = m.colwise();
auto it = std::find_if(cols.cbegin(), cols.cend(),
                       [](Matrix3i::ConstColXpr x) { return x.squaredNorm() == 0; });
cout << "The first empty column is: " << distance(cols.cbegin(),it) << endl;

}
  return 0;
}
