#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using eosio::asset;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::check;
using eosio::permission_level;
using eosio::current_time_point;
using eosio::action;
using eosio::same_payer;
using eosio::multi_index;
using eosio::indexed_by;
using eosio::const_mem_fun;
using std::string;
using std::vector;
using std::make_tuple;
using namespace std;

#define CONTRACTN name("clashdomedls")

class [[eosio::contract]] clashdomedls : public eosio::contract
{
    public:

        using contract::contract;

        ACTION create(uint64_t id, uint64_t type, uint64_t game, asset fee, uint64_t duration, string data);
        ACTION compromise(uint64_t id, name account);
        ACTION close(uint64_t id, name account, uint64_t score, uint64_t duration, uint64_t score2, name account2);
        ACTION claim(uint64_t id, name account);
        ACTION forceclaim(uint64_t id);
        ACTION reopen(uint64_t id);
        ACTION remove(uint64_t id);
        ACTION transaction(uint64_t id, string transactionId);
        ACTION setelo(name account, uint64_t game, uint64_t value);
        ACTION removeall();
        [[eosio::on_notify("eosio.token::transfer")]] void transfer(const name &from, const name &to, const asset &quantity, const string &memo);

    private:

        struct player_duel
        {
            name account;   // user wax account
            uint64_t timestamp;
            uint64_t duration;
            uint64_t score; // user score
            string data;
        };

        struct game_info {
            uint64_t id; 
            uint64_t total_duels;  
            uint64_t wins;
            uint64_t loses;
            uint64_t MMR;
        };

        TABLE player {
            name account;
            vector<game_info> games;
            uint64_t primary_key() const { return account.value; }
        };

        typedef multi_index<name("players"), player> players;

        TABLE duel {
            uint64_t id;
            uint64_t timestamp;
            uint64_t type;          // public 0 || private 1
            uint64_t state;         // open 0 || compromised 1 || closed 2 || claimed 3
            uint64_t game;          // endless-siege 0 || candy-fiesta 1 || ...
            string transaction;
            asset fee;              // entry fee for the duel
            player_duel player1;
            player_duel player2;

            uint64_t primary_key() const { return id; }
            uint64_t getDuel() const { return (game << 16) | state; }
            uint64_t getFirst() const { return player1.account.value; }
            uint64_t getSecond() const { return player2.account.value; }
            
        };

        typedef multi_index<name("duels"), duel, 
            indexed_by<name("getduel"), const_mem_fun<duel, uint64_t, &duel::getDuel>>,
            indexed_by<name("getfirst"), const_mem_fun<duel, uint64_t, &duel::getFirst>>,
            indexed_by<name("getsecond"), const_mem_fun<duel, uint64_t, &duel::getSecond>>>
        duels;

        enum DuelState {OPEN = 0, COMPROMISED, CLOSED, CLAIMED};
        enum GameType {ENDLESS_SIEGE = 0, CANDY_FIESTA};


        static constexpr name COMPANY_ACCOUNT = "gr.au.wam"_n;
        static constexpr name EOS_CONTRACT = "eosio.token"_n;
        static constexpr name LUDIO_CONTRACT = "clashdometkn"_n;
        static constexpr symbol WAX_SYMBOL = symbol(symbol_code("WAX"), 8);
        static constexpr symbol LUDIO_SYMBOL = symbol(symbol_code("LUDIO"), 4);
        static constexpr int64_t WAX_TO_LUDIO_RATIO = 1;

        uint64_t finder(vector<game_info> games, uint64_t id);
};