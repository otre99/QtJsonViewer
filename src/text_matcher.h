#ifndef TEXT_MATCHER_H
#define TEXT_MATCHER_H
#include <string>

using namespace std;

class TextMatcher {
public:
  TextMatcher(const string &s, bool caseSensitive, bool wholeWord, bool useRegex);

  bool match(const std::string &hay) const;

private:
  string m_msg;
  bool m_caseSensitive;
  bool m_wholeWord;
};

#endif // TEXT_MATCHER_H
