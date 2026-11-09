#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <fstream>
#include <random>
#include <cmath>
#include <chrono>

const int NO_ASK = std::numeric_limits<int>::max();
const double CANCEL_DIST = 1000;

struct Order
{
    int id;
    bool is_buy;
    int price;
    int quantity;
};

struct Trade
{
    int buy_id;
    int sell_id;
    int price;
    int quantity;
    double mid;
};

struct Book
{
    std::map<int, std::vector<Order>> bids;
    std::map<int, std::vector<Order>> asks;
};

void print_order(const Order &o)
{
    std::string buy_or_sell_string = "";
    if (o.is_buy)
    {
        buy_or_sell_string = "BUY";
    }
    else
    {
        buy_or_sell_string = "SELL";
    }
    std::cout << "#" << o.id << " " << buy_or_sell_string << " " << o.quantity << " @ " << o.price << std::endl;
}

int get_best_bid(const Book &book)
{
    if (book.bids.empty())
    {
        return 0;
    }
    else
    {
        return book.bids.rbegin()->first;
    }
}

int get_best_ask(const Book &book)
{
    if (book.asks.empty())
    {
        return NO_ASK;
    }
    else
    {
        return book.asks.begin()->first;
    }
}

void print_summary(const Book &book)
{
    int best_bid = get_best_bid(book);
    int best_ask = get_best_ask(book);
    int spread = best_ask - best_bid;
    if (best_bid == 0)
    {
        std::cout << "Best Bid: none" << std::endl;
    }
    else
    {
        std::cout << "Best Bid: " << best_bid / 100.0 << std::endl;
    }
    if (best_ask == NO_ASK)
    {
        std::cout << "Best Ask: none" << std::endl;
    }
    else
    {
        std::cout << "Best Ask: " << best_ask / 100.0 << std::endl;
    }
    if (best_bid == 0 || best_ask == NO_ASK)
    {
        std::cout << "Spread: none" << std::endl;
    }
    else
    {
        std::cout << "Spread: " << spread / 100.0 << std::endl;
    }
}

void add_order(Book &book, const Order &o, std::vector<Trade> &trades, double mid)
{
    Order incoming = o;
    if (incoming.is_buy)
    {
        while (incoming.quantity > 0 && !book.asks.empty())
        {
            int best_price = book.asks.begin()->first;
            if (incoming.price < best_price)
            {
                break;
            }
            std::vector<Order> &level = book.asks[best_price];
            Order &resting = level.front();

            int trade_qty = std::min(incoming.quantity, resting.quantity);
            trades.push_back({incoming.id, resting.id, best_price, trade_qty, mid});
            resting.quantity -= trade_qty;
            incoming.quantity -= trade_qty;

            if (resting.quantity == 0)
            {
                level.erase(level.begin());
            }
            if (level.empty())
            {
                book.asks.erase(best_price);
            }
        }
        if (incoming.quantity > 0)
        {
            book.bids[incoming.price].push_back(incoming);
        }
    }
    else
    {
        while (incoming.quantity > 0 && !book.bids.empty())
        {
            int best_price = book.bids.rbegin()->first;
            if (incoming.price > best_price)
            {
                break;
            }
            std::vector<Order> &level = book.bids[best_price];
            Order &resting = level.front();

            int trade_qty = std::min(incoming.quantity, resting.quantity);
            trades.push_back({resting.id, incoming.id, best_price, trade_qty, mid});
            resting.quantity -= trade_qty;
            incoming.quantity -= trade_qty;

            if (resting.quantity == 0)
            {
                level.erase(level.begin());
            }
            if (level.empty())
            {
                book.bids.erase(best_price);
            }
        }
        if (incoming.quantity > 0)
        {
            book.asks[incoming.price].push_back(incoming);
        }
    }
}

void write_csv(const std::vector<Trade> &trades)
{
    std::ofstream file("trades.csv");
    file << "buy_id,sell_id,price,quantity,mid" << std::endl;
    for (const Trade &t : trades)
    {
        file << t.buy_id << "," << t.sell_id << "," << t.price / 100.0 << "," << t.quantity << "," << t.mid << std::endl;
    }
}

void cancel_stale(Book& book, double mid)
{
    while (!book.asks.empty() && std::abs(book.asks.begin()->first / 100.0 - mid) > CANCEL_DIST)
    {
        book.asks.erase(book.asks.begin());
    }
    while (!book.asks.empty() && std::abs(book.asks.rbegin()->first / 100.0 - mid) > CANCEL_DIST)
    {
        book.asks.erase(std::prev(book.asks.end()));
    }
    while (!book.bids.empty() && std::abs(book.bids.begin()->first / 100.0 - mid) > CANCEL_DIST)
    {
        book.bids.erase(book.bids.begin());
    }
    while (!book.bids.empty() && std::abs(book.bids.rbegin()->first / 100.0 - mid) > CANCEL_DIST)
    {
        book.bids.erase(std::prev(book.bids.end()));
    }
}

int count_resting(const Book &book)
{
    int count = 0;
    for (const auto &[price, orders] : book.bids)
    {
        count += orders.size();
    }
    for (const auto &[price, orders] : book.asks)
    {
        count += orders.size();
    }
    return count;
}

int main()
{
    Book book;
    std::vector<Trade> trades;
    std::mt19937 rng(42);
    std::bernoulli_distribution side_dist(0.5);
    std::normal_distribution<double> price_dist(0.0, 0.5);
    std::normal_distribution<double> step_dist(0.0, 0.05);
    std::uniform_int_distribution<int> qty_dist(1, 20);
    double mid = 100.0;
    int num_orders = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= num_orders; i++)
    {
        mid += step_dist(rng);
        double p = price_dist(rng) + mid;
        int price_ticks = static_cast<int>(std::round(p * 100));
        Order o = {
            i,
            side_dist(rng),
            price_ticks,
            qty_dist(rng),
        };
        add_order(book, o, trades, mid);
        cancel_stale(book, mid);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "Time: " << ms << " ms" << std::endl;
    print_summary(book);
    std::cout << "Trades: " << trades.size() << std::endl;
    write_csv(trades);
    std::cout << "Resting orders: " << count_resting(book) << std::endl;
    return 0;
}