#ifndef ARGS_ARGS_HH_
#define ARGS_ARGS_HH_

#include <vector>
#include <string>
#include <map>
#include <algorithm>

namespace args {

class Parser {
public:
  using value_type = std::vector<std::string>;
  Parser(int argc, char** argv);
  std::string next();
  std::tuple<bool, std::string> find(const std::string& key) const;
  const value_type& data() const;
  Parser& require(const std::string& key);
  Parser& optional(const std::string& key);
  bool validate() const;
  std::string get(const std::string& key) const;
protected:
  value_type data_;
  std::size_t counter_ = 1;
  //! map<key, tuple<required, key found, value>>
  std::map<std::string, std::tuple<bool, bool, std::string>> dashed_;
private:
};

} /// namespace args

namespace args {

Parser::Parser(int argc, char** argv) {
  for (auto i = 0; i < argc; i++) {
    data_.emplace_back(argv[i]);
  }
}

std::string Parser::next() {
  if (counter_ < data_.size()) {
    return { data_[counter_++] };
  } else {
    return {};
  }
}

std::tuple<bool, std::string> Parser::find(const std::string& key) const {
  for (auto i = 0u; i < data_.size(); i++) {
    if (data_[i] == key) {
      if ((i + 1) < data_.size()) {
        return {true, data_[i + 1]};
      } else {
        return {true, {}};
      }
    }
  }
  return {false, {}};
}

const Parser::value_type& Parser::data() const { return data_; }

Parser& Parser::require(const std::string& key) {
  const auto found = this->find(key);
  dashed_.insert({key, {
      true,
      std::get<bool>(found),
      std::get<std::string>(found)
  }});
  return *this;
}

Parser& Parser::optional(const std::string& key) {
  const auto found = this->find(key);
  dashed_.insert({key, {
      false,
      std::get<bool>(found),
      std::get<std::string>(found)
  }});
  return *this;
}

bool Parser::validate() const {
  for (const auto& pair : dashed_) {
    if (
        (std::get<0>(pair.second) || std::get<1>(pair.second))
        && std::get<std::string>(pair.second).empty()
    ) {
      return false;
    }
  }
  const auto nonEmpty = static_cast<std::size_t>(std::count_if(
      dashed_.begin(),
      dashed_.end(),
      [](const auto& pair) {
        return !std::get<std::string>(pair.second).empty();
      }
  ));
  if (data_.size() != (nonEmpty * 2 + 1 + 1)) {
    return false;
  }
  return true;
}

std::string Parser::get(const std::string& key) const {
  return std::get<std::string>(dashed_.at(key));
}

} /// namespace args

#endif /// ARGS_ARGS_HH_
