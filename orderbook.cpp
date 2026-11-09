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

const double NO_ASK = std::numeric_limits<double>::max();

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

double get_best_bid(const std::vector<Order> &orders)
{
    double best_bid = 0;
    for (const Order &o : orders)
    {
        if (o.is_buy && o.price > best_bid)
        {
            best_bid = o.price;
        }
    }
    return best_bid;
}

double get_best_ask(const std::vector<Order> &orders)
{
    double best_ask = NO_ASK;
    for (const Order &o : orders)
    {
        if (!o.is_buy && o.price < best_ask)
        {

            best_ask = o.price;
        }
    }
    return best_ask;
}

void print_summary(const std::vector<Order> &book)
{
    double best_bid = get_best_bid(book);
    double best_ask = get_best_ask(book);
    double spread = best_ask - best_bid;
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

int find_best_ask_index(const std::vector<Order> &book)
{
    double best_ask = NO_ASK;
    int best_index = -1;

    for (int i = 0; i < book.size(); i++)
    {
        const Order &o = book[i];

        if (!o.is_buy && o.price < best_ask)
        {
            best_ask = o.price;
            best_index = i;
        }
    }
    return best_index;
}

int find_best_bid_index(const std::vector<Order> &book)
{
    double best_bid = 0;
    int best_index = -1;

    for (int i = 0; i < book.size(); i++)
    {
        const Order &o = book[i];

        if (o.is_buy && o.price > best_bid)
        {
            best_bid = o.price;
            best_index = i;
        }
    }
    return best_index;
}

void add_order(std::vector<Order> &book, const Order &o, std::vector<Trade> &trades, double mid)
{
    // std::cout << "--- new ";
    // print_order(o);
    Order incoming = o;

    if (incoming.is_buy)
    {
        while (incoming.quantity > 0)
        {
            int ask_index = find_best_ask_index(book);
            if (ask_index == -1 || incoming.price < book[ask_index].price)
            {
                break;
            }

            int trade_qty = std::min(incoming.quantity, book[ask_index].quantity);
            // std::cout << "TRADE: " << trade_qty << " @ " << book[ask_index].price
            //<< " (#" << incoming.id << " with #" << book[ask_index].id << ")" << std::endl;
            Trade trade = {incoming.id, book[ask_index].id, book[ask_index].price, trade_qty, mid};
            trades.push_back(trade);
            book[ask_index].quantity -= trade_qty;
            if (book[ask_index].quantity == 0)
            {
                book.erase(book.begin() + ask_index);
            }
            incoming.quantity -= trade_qty;
        }
    }
    else
    {
        while (incoming.quantity > 0)
        {
            int bid_index = find_best_bid_index(book);
            if (bid_index == -1 || incoming.price > book[bid_index].price)
            {
                break;
            }

            int trade_qty = std::min(incoming.quantity, book[bid_index].quantity);
            // std::cout << "TRADE: " << trade_qty << " @ " << book[bid_index].price
            //<< " (#" << incoming.id << " with #" << book[bid_index].id << ")" << std::endl;
            Trade trade = {book[bid_index].id, incoming.id, book[bid_index].price, trade_qty, mid};
            trades.push_back(trade);
            book[bid_index].quantity -= trade_qty;
            if (book[bid_index].quantity == 0)
            {
                book.erase(book.begin() + bid_index);
            }
            incoming.quantity -= trade_qty;
        }
    }

    if (incoming.quantity > 0)
    {
        book.push_back(incoming);
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

void cancel_stale(std::vector<Order> &book, double mid)
{
    for (int i = book.size() - 1; i >= 0; i--)
    {
        if (std::abs(book[i].price / 100.0 - mid) > 0.5)
        {
            book.erase(book.begin() + i);
        }
    }
}

int main()
{
    std::vector<Order> book;
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
    std::cout << "Resting orders: " << book.size() << std::endl;
    return 0;
}