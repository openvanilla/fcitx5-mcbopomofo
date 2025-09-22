// Copyright (c) 2025 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "ByteBlockBackedDictionary.h"

namespace McBopomofo {

namespace {

const char* AdvanceToNextNonWhitespace(const char* ptr, const char* end) {
  while (ptr != end) {
    if (const char c = *ptr; c != ' ' && c != '\t') {
      break;
    }
    ++ptr;
  }
  return ptr;
}

const char* AdvanceToNextCRLF(const char* ptr, const char* end) {
  while (ptr != end) {
    if (const char c = *ptr; c == '\r' || c == '\n') {
      break;
    }
    ++ptr;
  }
  return ptr;
}

const char* AdvanceToNextContentCharacter(const char* ptr, const char* end,
                                          size_t& lineCounter) {
  while (ptr != end) {
    const char c = *ptr;

    if (c == '\n') {
      ++lineCounter;
    }

    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
      break;
    }
    ++ptr;
  }
  return ptr;
}

const char* AdvanceToNextNonContentCharacter(const char* ptr, const char* end) {
  while (ptr != end) {
    if (const char c = *ptr; c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      break;
    }
    ++ptr;
  }
  return ptr;
}

const char* FindFirstNULL(const char* ptr, const char* end,
                          size_t* firstLineNumber = nullptr) {
  const char* i = ptr;
  while (i != end) {
    if (*i == 0) {
      break;
    }
    ++i;
  }

  // Only count the line number if there is indeed a NULL.
  if (i != end && firstLineNumber != nullptr) {
    size_t lineCounter = 1;
    while (ptr != i) {
      if (*ptr == '\n') {
        ++lineCounter;
      }
      ++ptr;
    }
    *firstLineNumber = lineCounter;
  }

  return i;
}

bool IsCRLF(char c) { return c == '\n' || c == '\r'; }

bool IsWhitespace(char c) { return c == ' ' || c == '\t'; }

}  // namespace

void ByteBlockBackedDictionary::clear() {
  dict_.clear();
  issues_.clear();
}

bool ByteBlockBackedDictionary::parse(const char* block, size_t size,
                                      ColumnOrder columnOrder) {
  if (block == nullptr) {
    return false;
  }

  if (size == 0) {
    return false;
  }

  clear();

  // Special case if block is a null-ended C string. This is the only place
  // NUL is allowed.
  if (block[size - 1] == 0) {
    --size;
  }

  const char* ptr = block;
  const char* end = ptr + size;

  // Validate that no NULL characters are in the text.
  size_t errorAtLine = 0;
  const char* ctrlCharPtr = FindFirstNULL(ptr, end, &errorAtLine);
  if (ctrlCharPtr != end) {
    issues_.emplace_back(Issue::Type::NULL_CHARACTER_IN_TEXT, errorAtLine);
    return false;
  }

  size_t lineCounter = 1;

  if (columnOrder == ColumnOrder::KEY_THEN_VALUE) {
    while (ptr != end) {
      ptr = AdvanceToNextContentCharacter(ptr, end, lineCounter);
      if (ptr == end) {
        break;
      }

      if (*ptr == '#') {
        ptr = AdvanceToNextCRLF(ptr, end);
        continue;
      }

      const char* keyStart = ptr;
      ptr = AdvanceToNextNonContentCharacter(ptr, end);
      const char* keyEnd = ptr;

      ptr = AdvanceToNextNonWhitespace(ptr, end);
      if (ptr == end || IsCRLF(*ptr)) {
        if (issues_.size() < MAX_ISSUES) {
          issues_.emplace_back(Issue::Type::MISSING_SECOND_COLUMN, lineCounter);
        }

        continue;
      }

      const char* valueStart = ptr;
      ptr = AdvanceToNextCRLF(ptr, end);
      const char* valueEnd = ptr;

      if (valueEnd == valueStart) {
        if (issues_.size() < MAX_ISSUES) {
          issues_.emplace_back(Issue::Type::MISSING_SECOND_COLUMN, lineCounter);
        }
        continue;
      }

      // Strip the trailing space in the value. If valuePtr already equals
      // valueStart at this point, the following test won't be true, since
      // *valueStart is a content character.
      if (IsWhitespace(*(valueEnd - 1))) {
        // DO NOT MOVE the next assignment before the test. In other words,
        // do not do IsWhitespace(*valuePtr) first. Benchmarking showed that
        // this extra load upfront resulted in performance penalty on x86_64.
        const char* valuePtr = valueEnd - 1;
        do {
          --valuePtr;
        } while (IsWhitespace(*valuePtr) && valuePtr != valueStart);
        valueEnd = valuePtr + 1;

        // This may be an assertion, but let's just be safe.
        if (valuePtr == valueStart) {
          if (issues_.size() < MAX_ISSUES) {
            issues_.emplace_back(Issue::Type::MISSING_SECOND_COLUMN,
                                 lineCounter);
          }
          continue;
        }
      }

      std::string_view key(keyStart, keyEnd - keyStart);
      std::string_view value(valueStart, valueEnd - valueStart);
      dict_[key].emplace_back(value);
    }
  } else {
    while (ptr != end) {
      ptr = AdvanceToNextContentCharacter(ptr, end, lineCounter);
      if (ptr == end) {
        break;
      }

      if (*ptr == '#') {
        ptr = AdvanceToNextCRLF(ptr, end);
        continue;
      }

      const char* valueStart = ptr;
      ptr = AdvanceToNextNonContentCharacter(ptr, end);
      const char* valueEnd = ptr;

      ptr = AdvanceToNextNonWhitespace(ptr, end);
      if (ptr == end || IsCRLF(*ptr)) {
        if (issues_.size() < MAX_ISSUES) {
          issues_.emplace_back(Issue::Type::MISSING_SECOND_COLUMN, lineCounter);
        }
        continue;
      }

      const char* maybeKeyStart = ptr;
      ptr = AdvanceToNextNonContentCharacter(ptr, end);
      const char* maybeKeyEnd = ptr;
      if (maybeKeyStart == maybeKeyEnd) {
        if (issues_.size() < MAX_ISSUES) {
          issues_.emplace_back(Issue::Type::MISSING_SECOND_COLUMN, lineCounter);
        }
        continue;
      }

      while (ptr != end) {
        // Skip any trailing space
        if (IsWhitespace(*ptr)) {
          ptr = AdvanceToNextNonWhitespace(ptr, end);
        }

        if (ptr == end || IsCRLF(*ptr)) {
          // We have the real key-value columns, now break.
          break;
        }

        // More content incoming.
        valueEnd = maybeKeyEnd;
        maybeKeyStart = ptr;
        ptr = AdvanceToNextNonContentCharacter(ptr, end);
        maybeKeyEnd = ptr;
      }

      std::string_view key(maybeKeyStart, maybeKeyEnd - maybeKeyStart);
      std::string_view value(valueStart, valueEnd - valueStart);
      dict_[key].emplace_back(value);
    }
  }

  return true;
}

bool ByteBlockBackedDictionary::hasKey(const std::string_view& key) const {
  return dict_.find(key) != dict_.end();
}

std::vector<std::string_view> ByteBlockBackedDictionary::getValues(
    const std::string_view& key) const {
  const auto it = dict_.find(key);
  if (it == dict_.cend()) {
    return {};
  }
  return it->second;
}

}  // namespace McBopomofo
