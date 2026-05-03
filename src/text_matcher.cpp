#include "text_matcher.h"

TextMatcher::TextMatcher(const string &s, bool caseSensitive, bool wholeWord, bool useRegex)
    : m_msg{s}, m_caseSensitive{caseSensitive}, m_wholeWord{wholeWord} {}

bool TextMatcher::match(const std::string &hay) const {
  if (m_caseSensitive) { // case sensitive -> whole Word + Sub-String
    return m_wholeWord ? hay == m_msg : hay.find(m_msg) != string::npos;
  }
  else {
    if (m_wholeWord) { // case insensitive + whole word
      if (hay.size() != m_msg.size()) {
        return false;
      }
      for (size_t i = 0; i < m_msg.size(); ++i) {
        if (std::tolower((m_msg[i])) != std::tolower(hay[i])) {
          return false;
        }
      }
      return true;
    }
    else { // case insensitive + substring
      size_t sub_len = m_msg.size();
      size_t str_len = hay.size();
      if (str_len < sub_len) {
        return false;
      }
      for (size_t i = 0; i <= str_len - sub_len; ++i) {
        bool match = true;
        for (size_t j = 0; j < sub_len; ++j) {
          if (std::tolower(hay[i + j]) != std::tolower(m_msg[j])) {
            match = false;
            break;
          }
        }
        if (match) {
          return true;
        }
      }
      return false;
    }
  }
}
