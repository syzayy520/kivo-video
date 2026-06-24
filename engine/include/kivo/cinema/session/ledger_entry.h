#pragma once

#include <string>

struct LedgerEntry {
  std::string id;
  std::string session_id;
  unsigned long long sequence_number = 0;
  unsigned long long generation = 0;
  std::string kind;
  std::string reason;
};
