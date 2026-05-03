#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

using namespace std;

const int   TOTAL_TABLES = 10;
const float TAX_RATE = 0.17f;
const float SERVICE_FEE = 0.05f;
const string DATA_FOLDER = "data/";

// ─── Enums ────────────────────────────────────────────────────────────────────
enum class Screen {
    LOGIN, HOME, MENU, ORDER, BILLING,
    RESERVATION, CUSTOMERS, REPORTS, KITCHEN, FEEDBACK, QUEUE
};
enum class UserRole { NONE, STAFF, CUSTOMER };
enum class OrderStatus { PENDING, PREPARING, READY, SERVED };
enum class PaymentMethod { CASH, CARD };

// ─── Utility helpers ──────────────────────────────────────────────────────────
inline string intToStr(int n) {
    ostringstream oss; oss << n; return oss.str();
}
inline string floatToStr(float f) {
    ostringstream oss; oss << fixed << setprecision(2) << f; return oss.str();
}
inline string getTodayDate() {
    time_t now = time(0);
    tm ltm;
#ifdef _WIN32
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif
    ostringstream oss;
    oss << (1900 + ltm.tm_year) << "-"
        << setw(2) << setfill('0') << (1 + ltm.tm_mon) << "-"
        << setw(2) << setfill('0') << ltm.tm_mday;
    return oss.str();
}
inline string statusToStr(OrderStatus s) {
    if (s == OrderStatus::PENDING)   return "Pending";
    if (s == OrderStatus::PREPARING) return "Preparing";
    if (s == OrderStatus::READY)     return "Ready";
    if (s == OrderStatus::SERVED)    return "Served";
    return "Unknown";
}

// ─── Entity (abstract base) ───────────────────────────────────────────────────
class Entity {
protected:
    int    id;
    string name;
public:
    Entity(int id, string name) : id(id), name(name) {}
    virtual ~Entity() {}
    int    getId()   const { return id; }
    string getName() const { return name; }
    void   setName(const string& n) { name = n; }
    virtual string getType()    const = 0;
    virtual string getSummary() const = 0;
};

// ─── MenuItem ─────────────────────────────────────────────────────────────────
class MenuItem {
private:
    int    itemId;
    string itemName;
    float  price;
    string category;
    bool   available;
public:
    MenuItem(int id, string name, float price, string cat)
        : itemId(id), itemName(name), price(price), category(cat), available(true) {
    }

    int    getItemId()   const { return itemId; }
    string getItemName() const { return itemName; }
    float  getPrice()    const { return price; }
    string getCategory() const { return category; }
    bool   isAvailable() const { return available; }

    void setPrice(float p) { price = p; }
    void setAvailable(bool a) { available = a; }
    void setItemName(string n) { itemName = n; }

    string getDisplayLine() const {
        return "[" + intToStr(itemId) + "] " + itemName +
            " - Rs." + floatToStr(price) +
            "  [" + category + "]" +
            (available ? "" : " (Unavailable)");
    }
};

// ─── OrderItem ────────────────────────────────────────────────────────────────
struct OrderItem {
    MenuItem item;
    int      quantity;
    OrderItem(MenuItem i, int q) : item(i), quantity(q) {}
    float getSubtotal() const { return item.getPrice() * quantity; }
};

// ─── Order ────────────────────────────────────────────────────────────────────
class Order {
private:
    int               orderId;
    int               tableNumber;
    vector<OrderItem> items;
    OrderStatus       status;
    string            orderDate;
    bool              paid;
    PaymentMethod     payMethod;
    string            customerName;
public:
    Order(int id, int table, string custName)
        : orderId(id), tableNumber(table), status(OrderStatus::PENDING),
        paid(false), payMethod(PaymentMethod::CASH), customerName(custName) {
        orderDate = getTodayDate();
    }

    int         getOrderId()      const { return orderId; }
    int         getTableNumber()  const { return tableNumber; }
    OrderStatus getStatus()       const { return status; }
    string      getOrderDate()    const { return orderDate; }
    bool        isPaid()          const { return paid; }
    string      getCustomerName() const { return customerName; }
    const vector<OrderItem>& getItems() const { return items; }

    void addItem(const MenuItem& menuItem, int qty) {
        for (auto& oi : items) {
            if (oi.item.getItemId() == menuItem.getItemId()) {
                oi.quantity += qty;
                return;
            }
        }
        items.push_back(OrderItem(menuItem, qty));
    }

    void removeItem(int itemId) {
        items.erase(
            remove_if(items.begin(), items.end(),
                [itemId](const OrderItem& oi) { return oi.item.getItemId() == itemId; }),
            items.end());
    }

    float getSubtotal()     const { float t = 0; for (const auto& o : items) t += o.getSubtotal(); return t; }
    float getTax()          const { return getSubtotal() * TAX_RATE; }
    float getServiceCharge()const { return getSubtotal() * SERVICE_FEE; }
    float getTotal()        const { return getSubtotal() + getTax() + getServiceCharge(); }

    void advanceStatus() {
        if (status == OrderStatus::PENDING)   status = OrderStatus::PREPARING;
        else if (status == OrderStatus::PREPARING) status = OrderStatus::READY;
        else if (status == OrderStatus::READY)     status = OrderStatus::SERVED;
    }
    void markPaid(PaymentMethod method) { paid = true; payMethod = method; }
};

// ─── Customer ─────────────────────────────────────────────────────────────────
class Customer : public Entity {
private:
    string phone;
    int    visits;
    float  totalSpent;
    int    feedback;
    string username;
    string password;
public:
    Customer(int id, string name, string phone,
        string user = "", string pass = "")
        : Entity(id, name), phone(phone), visits(0),
        totalSpent(0.0f), feedback(0), username(user), password(pass) {
    }

    string getPhone()      const { return phone; }
    int    getVisits()     const { return visits; }
    float  getTotalSpent() const { return totalSpent; }
    int    getFeedback()   const { return feedback; }
    string getUsername()   const { return username; }
    string getPassword()   const { return password; }
    bool   hasLogin()      const { return !username.empty(); }

    void setPhone(const string& p) { phone = p; }
    void setFeedback(int rating) { feedback = rating; }
    void setUsername(const string& u) { username = u; }
    void setPassword(const string& p) { password = p; }
    void setVisits(int v) { visits = v; }
    void setTotalSpent(float s) { totalSpent = s; }

    bool checkPassword(const string& pass) const { return password == pass; }

    void recordVisit(float amount) { visits++; totalSpent += amount; }

    string getType()    const override { return "Customer"; }
    string getSummary() const override {
        return name + " | Ph: " + phone +
            " | Visits: " + intToStr(visits) +
            " | Total: Rs." + floatToStr(totalSpent) +
            (hasLogin() ? " [App User]" : "");
    }
};

// ─── Staff ────────────────────────────────────────────────────────────────────
class Staff : public Entity {
private:
    string role;
    string username;
    string password;
    float  salary;
public:
    Staff(int id, string name, string role,
        string user, string pass, float sal)
        : Entity(id, name), role(role), username(user), password(pass), salary(sal) {
    }

    string getRole()     const { return role; }
    string getUsername() const { return username; }
    float  getSalary()   const { return salary; }
    bool checkPassword(const string& pass) const { return password == pass; }

    string getType()    const override { return "Staff"; }
    string getSummary() const override {
        return name + " | Role: " + role + " | User: " + username;
    }
};

// ─── Table ────────────────────────────────────────────────────────────────────
class Table {
private:
    int    tableNum;
    int    capacity;
    bool   reserved;
    string reservedFor;
    string reservationTime;
public:
    Table(int num, int cap) : tableNum(num), capacity(cap), reserved(false) {}

    int    getTableNum()        const { return tableNum; }
    int    getCapacity()        const { return capacity; }
    bool   isReserved()         const { return reserved; }
    string getReservedFor()     const { return reservedFor; }
    string getReservationTime() const { return reservationTime; }

    void reserve(const string& name, const string& timeSlot) {
        reserved = true; reservedFor = name; reservationTime = timeSlot;
    }
    void cancelReservation() {
        reserved = false; reservedFor = ""; reservationTime = "";
    }
};

// ─── FeedbackRecord ───────────────────────────────────────────────────────────
struct FeedbackRecord {
    string customerName;
    int    rating;
    string comment;
    string date;
    FeedbackRecord(string cn, int r, string cmt)
        : customerName(cn), rating(r), comment(cmt) {
        date = getTodayDate();
    }
};

// ─── WaitingEntry ─────────────────────────────────────────────────────────────
struct WaitingEntry {
    string name;
    int    partySize;
    string arrivalTime;
    WaitingEntry(string n, int ps, string at)
        : name(n), partySize(ps), arrivalTime(at) {
    }
};

// ─── FileHandler ──────────────────────────────────────────────────────────────
class FileHandler {
public:
    static void saveMenu(const vector<MenuItem>& menu) {
        ofstream file(DATA_FOLDER + "menu.txt");
        if (!file.is_open()) return;
        for (const auto& item : menu)
            file << item.getItemId() << "|" << item.getItemName() << "|"
            << item.getPrice() << "|" << item.getCategory() << "|"
            << item.isAvailable() << "\n";
        file.close();
    }

    static vector<MenuItem> loadMenu() {
        vector<MenuItem> menu;
        ifstream file(DATA_FOLDER + "menu.txt");
        if (!file.is_open()) return menu;
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string id, name, price, cat, avail;
            getline(ss, id, '|'); getline(ss, name, '|');
            getline(ss, price, '|'); getline(ss, cat, '|');
            getline(ss, avail, '|');
            if (id.empty()) continue;
            MenuItem item(stoi(id), name, stof(price), cat);
            item.setAvailable(avail == "1");
            menu.push_back(item);
        }
        file.close();
        return menu;
    }

    static void saveCustomers(const vector<Customer>& customers) {
        ofstream file(DATA_FOLDER + "customers.txt");
        if (!file.is_open()) return;
        for (const auto& c : customers)
            file << c.getId() << "|" << c.getName() << "|"
            << c.getPhone() << "|" << c.getVisits() << "|"
            << c.getTotalSpent() << "|" << c.getFeedback() << "|"
            << c.getUsername() << "|" << c.getPassword() << "\n";
        file.close();
    }

    static vector<Customer> loadCustomers() {
        vector<Customer> customers;
        ifstream file(DATA_FOLDER + "customers.txt");
        if (!file.is_open()) return customers;
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string id, name, phone, visits, spent, fb, user, pass;
            getline(ss, id, '|'); getline(ss, name, '|');
            getline(ss, phone, '|'); getline(ss, visits, '|');
            getline(ss, spent, '|'); getline(ss, fb, '|');
            getline(ss, user, '|'); getline(ss, pass, '|');
            if (id.empty()) continue;
            Customer c(stoi(id), name, phone, user, pass);
            c.setVisits(stoi(visits));
            c.setTotalSpent(stof(spent));
            c.setFeedback(stoi(fb));
            customers.push_back(c);
        }
        file.close();
        return customers;
    }

    static void saveOrderHistory(const Order& order) {
        ofstream file(DATA_FOLDER + "orders.txt", ios::app);
        if (!file.is_open()) return;
        file << order.getOrderId() << "|" << order.getCustomerName() << "|"
            << order.getTableNumber() << "|" << order.getTotal() << "|"
            << order.getOrderDate() << "\n";
        file.close();
    }

    static float loadTodaysSales() {
        float total = 0;
        ifstream file(DATA_FOLDER + "orders.txt");
        if (!file.is_open()) return 0;
        string today = getTodayDate(), line;
        while (getline(file, line)) {
            stringstream ss(line);
            string id, cust, table, amount, date;
            getline(ss, id, '|'); getline(ss, cust, '|');
            getline(ss, table, '|'); getline(ss, amount, '|'); getline(ss, date, '|');
            if (date == today && !amount.empty()) total += stof(amount);
        }
        file.close();
        return total;
    }

    static int loadTodaysOrderCount() {
        int count = 0;
        ifstream file(DATA_FOLDER + "orders.txt");
        if (!file.is_open()) return 0;
        string today = getTodayDate(), line;
        while (getline(file, line)) {
            stringstream ss(line);
            string id, cust, table, amount, date;
            getline(ss, id, '|'); getline(ss, cust, '|');
            getline(ss, table, '|'); getline(ss, amount, '|'); getline(ss, date, '|');
            if (date == today) count++;
        }
        file.close();
        return count;
    }

    static void ensureDataFolder() {
        ofstream test(DATA_FOLDER + ".keep");
        if (!test.is_open()) system("mkdir -p data");
        else test.close();
    }
};