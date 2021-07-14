duels-smart-contract

# Build
eosio-cpp clashdomedls.cpp -o clashdomedls.wasm

# Update contract

While testing in testnet:
cleos -u https://testnet.waxsweden.org set contract clashdomedls ./clashdomedls -p clashdomedls@active

In production:
cleos -u https://api.waxsweden.org set contract clashdomedls ./clashdomedls -p clashdomedls@active