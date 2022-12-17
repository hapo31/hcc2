#!/bin/bash

count=0
success_count=0
fail_count=0
assert() {
  expected="$1"
  input="$2"

  ./hcc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
    ((success_count += 1))
  else
    echo "$input => $expected expected, but got $actual"
    ((fail_count += 1))
  fi
  ((count += 1))
}

assert 0 0
assert 42 42
assert 21 '5+20-4'
assert 23 '5 + 20 - 2'
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 2 '-3+5'
assert 5 '-1+6'
assert 0 '-1+(+6)+(-5)'
assert 1 '1 == 1'
assert 1 '1 != 2'
assert 1 '2 >= 1'
assert 1 '1 <= 2'
assert 1 '1 < 2'
assert 1 '2 > 1'
assert 0 '1 == 2'
assert 0 '1 != 1'
assert 0 '1 >= 2'
assert 0 '2 <= 1'
assert 0 '2 < 1'
assert 0 '1 > 2'

if [ $success_count = $count ]; then
  echo "Tests All Green 🎉 (All tests count: $count)"; 
else 
  echo "$fail_count tests failed (All tests count: $count)";
  exit 1
fi