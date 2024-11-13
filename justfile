build:
  cmake --build build -j 17

configure:
  cmake -B build

bitcoind: build
  ./build/src/bitcoind -regtest

help:
  ./build/src/bitcoin-cli -regtest help api

api:
  ./build/src/bitcoin-cli -regtest api

test: build
  ctest --test-dir build -j 17

test-fast:
  cmake --build build --target test_bitcoin
  ctest --test-dir build -j 17

last-test:
  less build/Testing/Temporary/LastTest.log
