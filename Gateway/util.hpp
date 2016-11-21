#include <iostream>
// Bibliotecas Boost
#include <boost/regex.hpp>

using namespace std;
using namespace boost;

bool get_args(string msg, vector<string>& values)
{
	const char* pattern("-?[0-9]*\\.?[0-9]+");
	/*string content("/?id=189740&timestamp=1447347829"
	"&lat=-519.86979471&lon=-543.96226144"
	"&speed = 0.0&bearing = 0.0"
	"&altitude = 819.0&batt = 91.0");*/

	regex re(pattern);

	boost::sregex_iterator it(msg.begin(), msg.end(), re);
	boost::sregex_iterator end;

	for (int i = 0; it != end; ++it, i++)
	{
		values.push_back(it->str());
		cout << values[i] << endl;
	}

	if (values.empty()) return false;
	else return true;
}