#include "clashdomedls.hpp"

void clashdomedls::createduel(string state, string type, string game, asset fee)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    _dl.emplace(CONTRACTN, [&](auto& new_duel) {
        new_duel.id = _dl.available_primary_key();
        new_duel.timestamp = timestamp;
        new_duel.state = state;
        new_duel.type = type;
        new_duel.game = game;
        new_duel.fee = fee;
    });
}

void clashdomedls::updateelo(string game, name winner, name loser)
{
    require_auth(_self);

    players _pl(CONTRACTN, CONTRACTN.value);

    // WINNER
    auto pl_itr = _pl.find(winner.value);

    // new user
    if (pl_itr == _pl.end()) {

        vector<game_info> games;
        games.push_back({ game, 1, 1, 0, 1000 }); 

        _pl.emplace(CONTRACTN, [&](auto& new_player) {
            new_player.account = winner;
            new_player.games = games;
        });
    } else {

        uint64_t pos = finder(pl_itr->games, game);

        // new game
        if (pos == -1) {
            _pl.modify(pl_itr, get_self(), [&](auto &mod_player) {
                mod_player.games.push_back({ game, 1, 1, 0, 1000 });
            });
        } else {
            _pl.modify(pl_itr, get_self(), [&](auto &mod_player) {
                mod_player.games.at(pos).total_duels++;
                mod_player.games.at(pos).wins++;
            });
        }
    }
}

void clashdomedls::eraseall() {

    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);
    for (auto dl_itr = _dl.begin(); dl_itr != _dl.end();) {
        dl_itr = _dl.erase(dl_itr);
    }

    players _pl(CONTRACTN, CONTRACTN.value);
    for (auto pl_itr = _pl.begin(); pl_itr != _pl.end();) {
        pl_itr = _pl.erase(pl_itr);
    }
}

uint64_t clashdomedls::finder(vector<game_info> games, string id)
{
    for (uint64_t i = 0; i < games.size(); i++)
    {
        if (games.at(i).id == id)
        {
            return i;
        }
    }
    return -1;
}