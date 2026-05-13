#pragma once

// Bundu Khan Restaurant Management System
// Contains: Constants, Data Models, and FileHandler

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cctype>
using namespace std;


//  SECTION 1 — CONSTANTS & UTILITIES


// Window dimensions
const int WINDOW_WIDTH  = 1100;
const int WINDOW_HEIGHT = 700;

// System-wide capacity limits
const int TOTAL_TABLES    = 10;
const int MAX_MENU        = 50;
const int MAX_ORDERS      = 100;
const int MAX_CUSTOMERS   = 200;
const int MAX_STAFF       = 10;
const int MAX_FEEDBACKS   = 200;
const int MAX_QUEUE       = 50;
const int MAX_ORDER_ITEMS = 20;

// Tax and service fee applied to every order total
const float TAX_RATE    = 0.17f;
const float SERVICE_FEE = 0.05f;

const string DATA_FOLDER = "data/";

// Screen IDs — used to track which screen is currently active
const int SCR_LOGIN       = 0;
const int SCR_HOME        = 1;
const int SCR_MENU        = 2;
const int SCR_ORDER       = 3;
const int SCR_BILLING     = 4;
const int SCR_RESERVATION = 5;
const int SCR_CUSTOMERS   = 6;
const int SCR_REPORTS     = 7;
const int SCR_KITCHEN     = 8;
const int SCR_FEEDBACK    = 9;
const int SCR_QUEUE       = 10;

// Role IDs — determines what a logged-in user can access
const int ROLE_NONE     = 0;
const int ROLE_STAFF    = 1;
const int ROLE_CUSTOMER = 2;

// Order status progression: Pending → Preparing → Ready → Served
const int STATUS_PENDING   = 0;
const int STATUS_PREPARING = 1;
const int STATUS_READY     = 2;
const int STATUS_SERVED    = 3;

// Payment method IDs
const int PAY_CASH = 0;
const int PAY_CARD = 1;

// Converts int/float to string for display and file output
inline string intToStr(int n)   { ostringstream oss; oss << n; return oss.str(); }
inline string floatToStr(float f) { ostringstream oss; oss << fixed << setprecision(2) << f; return oss.str(); }

// Returns today's date as "YYYY-MM-DD" using platform-safe localtime
inline string getTodayDate() {
    time_t now = time(0); tm ltm;
#ifdef _WIN32
    localtime_s(<m, &now);
#else
    localtime_r(&now, <m);
#endif
    ostringstream oss;
    oss << (1900 + ltm.tm_year) << "-"
        << setw(2) << setfill('0') << (1 + ltm.tm_mon) << "-"
        << setw(2) << setfill('0') << ltm.tm_mday;
    return oss.str();
}

// Converts a status int to a human-readable string
inline string statusToStr(int s) {
    if (s == STATUS_PENDING)   return "Pending";
    if (s == STATUS_PREPARING) return "Preparing";
    if (s == STATUS_READY)     return "Ready";
    if (s == STATUS_SERVED)    return "Served";
    return "Unknown";
}

// Input validators — return true if the input meets the required format
inline bool isValidName(const string& s) {
    if ((int)s.size() < 2) return false;
    for (char c : s) if (!isalpha(c) && c != ' ') return false;
    return true;
}
inline bool isValidUsername(const string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!isalnum(c) && c != '_') return false;
    return true;
}
inline bool isValidPhone(const string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!isdigit(c)) return false;
    return true;
}
// Returns true if every digit in the phone number is the same (e.g. "0000000000")
inline bool isPhoneAllSame(const string& s) {
    for (int i = 1; i < (int)s.size(); i++)
        if (s[i] != s[0]) return false;
    return true;
}
// Safe wrappers around stoi/stof — return false instead of throwing on bad input
inline bool safeStoi(const string& s, int& out) {
    if (s.empty()) return false;
    try { out = stoi(s); return true; } catch (...) { return false; }
}
inline bool safeStof(const string& s, float& out) {
    if (s.empty()) return false;
    try { out = stof(s); return true; } catch (...) { return false; }
}
inline bool isValidCategory(const string& s) {
    if ((int)s.size() < 2) return false;
    for (char c : s) if (!isalpha(c) && c != ' ') return false;
    return true;
}
inline bool isValidComment(const string& s) { return (int)s.size() >= 3; }
inline bool isValidPrice(const string& s, float& out) {
    if (!safeStof(s, out)) return false;
    return (out > 0.0f && out <= 99999.0f);
}
inline bool isValidQty(const string& s, int& out) {
    if (!safeStoi(s, out)) return false;
    return (out >= 1 && out <= 50);
}
inline bool isValidPartySize(const string& s, int& out) {
    if (!safeStoi(s, out)) return false;
    return (out >= 1 && out <= 20);
}
// Validates time slots like "9AM", "12PM" — hour must be 1–12
inline bool isValidTimeSlot(const string& t) {
    int numEnd = 0;
    for (int i = 0; i < (int)t.size(); i++) { if (isdigit(t[i])) numEnd = i + 1; else break; }
    if (numEnd == 0 || numEnd > 2) return false;
    string suffix = t.substr(numEnd);
    for (char& c : suffix) c = toupper(c);
    if (suffix != "AM" && suffix != "PM") return false;
    int hour = stoi(t.substr(0, numEnd));
    return (hour >= 1 && hour <= 12);
}
// Validates date format "YYYY-MM-DD"
inline bool isValidDate(const string& d) {
    if (d.size() != 10) return false;
    if (d[4] != '-' || d[7] != '-') return false;
    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !isdigit(d[i])) return false;
    return true;
}

// Generates a checksum from order ID, amount, and table number.
// Used to detect tampered records in the orders log.
inline long long computeChecksum(int orderId, float amount, int tableNum) {
    return (long long)(orderId) * 7LL
        + (long long)(amount * 100.0f) * 13LL
        + (long long)(tableNum) * 3LL;
}


//  SECTION 2 — DATA MODELS


// Represents a single item on the restaurant menu
class MenuItem {
private:
    int    itemId;
    string itemName;
    float  price;
    string category;
    bool   available;
public:
    MenuItem() : itemId(0), itemName(""), price(0), category(""), available(true) {}
    MenuItem(int id, string name, float p, string cat)
        : itemId(id), itemName(name), price(p), category(cat), available(true) {}

    int    getItemId()   const { return itemId; }
    string getItemName() const { return itemName; }
    float  getPrice()    const { return price; }
    string getCategory() const { return category; }
    bool   isAvailable() const { return available; }

    void setPrice(float p)      { price = p; }
    void setAvailable(bool a)   { available = a; }
    void setItemName(string n)  { itemName = n; }

    // Returns a formatted one-line string for display in the menu list
    string getDisplayLine() const {
        return "[" + intToStr(itemId) + "] " + itemName +
            " - Rs." + floatToStr(price) +
            "  [" + category + "]" +
            (available ? "" : " (Unavailable)");
    }
};

// Represents one menu item and its quantity within an order
class OrderItem {
private:
    MenuItem item;
    int      quantity;
public:
    OrderItem() : quantity(0) {}
    OrderItem(MenuItem i, int q) : item(i), quantity(q) {}

    const MenuItem& getItem()     const { return item; }
    int             getQuantity() const { return quantity; }
    float           getSubtotal() const { return item.getPrice() * quantity; }

    // Used when the same item is added to an order more than once
    void addQuantity(int q) { quantity += q; }
};

// Represents a full customer order containing multiple items
class Order {
private:
    int       orderId;
    int       tableNumber;
    OrderItem items[MAX_ORDER_ITEMS];
    int       itemCount;
    int       status;
    string    orderDate;
    bool      paid;
    bool      awaitingCashConfirm;
    int       payMethod;
    string    customerName;
public:
    Order() : orderId(0), tableNumber(0), itemCount(0),
        status(STATUS_PENDING), paid(false),
        awaitingCashConfirm(false), payMethod(PAY_CASH) {
        orderDate = getTodayDate();
    }
    Order(int id, int table, string custName)
        : orderId(id), tableNumber(table), itemCount(0),
        status(STATUS_PENDING), paid(false),
        awaitingCashConfirm(false), payMethod(PAY_CASH),
        customerName(custName) {
        orderDate = getTodayDate();
    }

    int    getOrderId()             const { return orderId; }
    int    getTableNumber()         const { return tableNumber; }
    int    getStatus()              const { return status; }
    string getOrderDate()           const { return orderDate; }
    bool   isPaid()                 const { return paid; }
    string getCustomerName()        const { return customerName; }
    int    getItemCount()           const { return itemCount; }
    const OrderItem& getItem(int i) const { return items[i]; }

    // Adds an item to the order; merges quantity if item already exists
    void addItem(const MenuItem& menuItem, int qty) {
        for (int i = 0; i < itemCount; i++) {
            if (items[i].getItem().getItemId() == menuItem.getItemId()) {
                items[i].addQuantity(qty); return;
            }
        }
        if (itemCount < MAX_ORDER_ITEMS)
            items[itemCount++] = OrderItem(menuItem, qty);
    }

    // Removes an item by its menu item ID, shifting remaining items left
    void removeItem(int menuItemId) {
        for (int i = 0; i < itemCount; i++) {
            if (items[i].getItem().getItemId() == menuItemId) {
                for (int j = i; j < itemCount - 1; j++) items[j] = items[j + 1];
                itemCount--; return;
            }
        }
    }

    // Bill calculations — tax and service charge are applied on top of subtotal
    float getSubtotal()      const { float t = 0; for (int i = 0; i < itemCount; i++) t += items[i].getSubtotal(); return t; }
    float getTax()           const { return getSubtotal() * TAX_RATE; }
    float getServiceCharge() const { return getSubtotal() * SERVICE_FEE; }
    float getTotal()         const { return getSubtotal() + getTax() + getServiceCharge(); }

    // Moves the order to the next status stage
    void advanceStatus() {
        if      (status == STATUS_PENDING)   status = STATUS_PREPARING;
        else if (status == STATUS_PREPARING) status = STATUS_READY;
        else if (status == STATUS_READY)     status = STATUS_SERVED;
    }

    void markPaid(int method)    { paid = true; awaitingCashConfirm = false; payMethod = method; }
    bool isAwaitingCashConfirm() const { return awaitingCashConfirm; }
    void requestCashPayment()    { awaitingCashConfirm = true; payMethod = PAY_CASH; }
    void confirmCashPayment()    { paid = true; awaitingCashConfirm = false; }
};

// Stores a registered customer's profile, visit history, and login credentials
class Customer {
private:
    int    id;
    string name, phone, username, password;
    int    visits;
    float  totalSpent;
    int    feedback;
public:
    Customer() : id(0), visits(0), totalSpent(0), feedback(0) {}
    Customer(int id, string name, string phone, string user = "", string pass = "")
        : id(id), name(name), phone(phone), visits(0), totalSpent(0),
        feedback(0), username(user), password(pass) {}

    int    getId()         const { return id; }
    string getName()       const { return name; }
    string getPhone()      const { return phone; }
    int    getVisits()     const { return visits; }
    float  getTotalSpent() const { return totalSpent; }
    int    getFeedback()   const { return feedback; }
    string getUsername()   const { return username; }
    bool   hasLogin()      const { return !username.empty(); }

    void setVisits(int v)          { visits = (v >= 0) ? v : 0; }
    void setTotalSpent(float s)    { totalSpent = (s >= 0) ? s : 0; }
    void setFeedback(int r)        { feedback = (r >= 1 && r <= 5) ? r : 0; }
    void setPhone(const string& p) { phone = p; }

    bool checkPassword(const string& pass) const { return password == pass; }
    void recordVisit(float amount) { visits++; totalSpent += amount; }

    // Returns a formatted summary line for the customer list screen
    string getSummary() const {
        return name + " | Ph: " + phone +
            " | Visits: " + intToStr(visits) +
            " | Total: Rs." + floatToStr(totalSpent) +
            (hasLogin() ? " [App User]" : "");
    }
};

// Stores a staff member's credentials and role
class Staff {
private:
    int    id;
    string name, role, username, password;
    float  salary;
public:
    Staff() : id(0), salary(0) {}
    Staff(int id, string name, string role, string user, string pass, float sal)
        : id(id), name(name), role(role), username(user), password(pass), salary(sal) {}

    int    getId()       const { return id; }
    string getName()     const { return name; }
    string getRole()     const { return role; }
    string getUsername() const { return username; }
    float  getSalary()   const { return salary; }
    bool   checkPassword(const string& p) const { return password == p; }
};

// Tracks a single dining table — its capacity and reservation status
class Table {
private:
    int    tableNum, capacity;
    bool   reserved;
    string reservedFor, reservationTime;
public:
    Table() : tableNum(0), capacity(0), reserved(false) {}
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

// Stores a single customer feedback entry with a rating and comment
class FeedbackRecord {
private:
    string customerName;
    string comment;
    string date;
    int    rating;
public:
    FeedbackRecord() : rating(0) {}
    FeedbackRecord(string cn, int r, string cmt)
        : customerName(cn), rating(r), comment(cmt) {
        date = getTodayDate();
    }

    string getCustomerName() const { return customerName; }
    string getComment()      const { return comment; }
    string getDate()         const { return date; }
    int    getRating()       const { return rating; }
    void   setDate(const string& d) { date = d; }
};

// Represents a customer waiting in the queue for a table
class WaitingEntry {
private:
    string name;
    string arrivalTime;
    int    partySize;
public:
    WaitingEntry() : partySize(0) {}
    WaitingEntry(string n, int ps, string at)
        : name(n), partySize(ps), arrivalTime(at) {}

    string getName()        const { return name; }
    string getArrivalTime() const { return arrivalTime; }
    int    getPartySize()   const { return partySize; }
};

// Stores a saved order record used to verify billing integrity
class IntegrityRecord {
private:
    int       orderId;
    float     amount;
    int       tableNum;
    string    date;
    long long checksum;
public:
    IntegrityRecord() : orderId(0), amount(0.0f), tableNum(0), checksum(0) {}

    int       getOrderId()  const { return orderId; }
    float     getAmount()   const { return amount; }
    int       getTableNum() const { return tableNum; }
    string    getDate()     const { return date; }
    long long getChecksum() const { return checksum; }

    void setOrderId(int id)          { orderId = id; }
    void setAmount(float a)           { amount = a; }
    void setTableNum(int t)           { tableNum = t; }
    void setDate(const string& d)    { date = d; }
    void setChecksum(long long cs)   { checksum = cs; }
};

//  SECTION 3 — FILE HANDLER

// Handles all reading and writing of application data to disk.
// All files are stored as pipe-delimited text in the data/ folder.
class FileHandler {
public:

    // Creates the data/ folder if it does not already exist
    static void ensureDataFolder() {
        ofstream test(DATA_FOLDER + ".keep");
        if (!test.is_open()) system("mkdir -p data");
        else test.close();
    }

    // Writes all menu items to menu.txt
    static void saveMenu(MenuItem menu[], int count) {
        ofstream file(DATA_FOLDER + "menu.txt");
        if (!file.is_open()) return;
        for (int i = 0; i < count; i++)
            file << menu[i].getItemId() << "|"
            << menu[i].getItemName() << "|"
            << menu[i].getPrice() << "|"
            << menu[i].getCategory() << "|"
            << menu[i].isAvailable() << "\n";
        file.close();
    }

    // Reads menu items from menu.txt; skips malformed or invalid lines
    static int loadMenu(MenuItem menu[], int maxCount) {
        int count = 0;
        ifstream file(DATA_FOLDER + "menu.txt");
        if (!file.is_open()) return 0;
        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, name, price, cat, avail;
            getline(ss, id, '|'); getline(ss, name, '|');
            getline(ss, price, '|'); getline(ss, cat, '|'); getline(ss, avail, '|');
            if (id.empty() || name.empty() || price.empty() || cat.empty()) continue;
            int parsedId; float parsedPrice;
            if (!safeStoi(id, parsedId))       continue;
            if (!safeStof(price, parsedPrice)) continue;
            if (parsedPrice <= 0.0f || parsedPrice > 99999.0f) continue;
            if (name.size() < 2 || cat.size() < 2) continue;
            MenuItem item(parsedId, name, parsedPrice, cat);
            item.setAvailable(avail == "1");
            menu[count++] = item;
        }
        file.close();
        return count;
    }

    // Writes all customers including their plaintext passwords to customers.txt
    static void saveCustomersFull(Customer customers[], int count, string passwords[]) {
        ofstream file(DATA_FOLDER + "customers.txt");
        if (!file.is_open()) return;
        for (int i = 0; i < count; i++)
            file << customers[i].getId() << "|"
            << customers[i].getName() << "|"
            << customers[i].getPhone() << "|"
            << customers[i].getVisits() << "|"
            << customers[i].getTotalSpent() << "|"
            << customers[i].getFeedback() << "|"
            << customers[i].getUsername() << "|"
            << passwords[i] << "\n";
        file.close();
    }

    // Reads customers from file; resets out-of-range values to safe defaults
    static int loadCustomers(Customer customers[], int maxCount, string passwords[]) {
        int count = 0;
        ifstream file(DATA_FOLDER + "customers.txt");
        if (!file.is_open()) return 0;
        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, name, phone, visits, spent, fb, user, pass;
            getline(ss, id, '|'); getline(ss, name, '|'); getline(ss, phone, '|');
            getline(ss, visits, '|'); getline(ss, spent, '|'); getline(ss, fb, '|');
            getline(ss, user, '|'); getline(ss, pass, '|');
            if (id.empty() || name.empty() || phone.empty() || user.empty() || pass.empty()) continue;
            int parsedId, parsedVisits, parsedFb; float parsedSpent;
            if (!safeStoi(id, parsedId)) continue;
            if (!safeStoi(visits, parsedVisits)) parsedVisits = 0;
            if (!safeStof(spent, parsedSpent))   parsedSpent = 0;
            if (!safeStoi(fb, parsedFb))         parsedFb = 0;
            if (parsedVisits < 0) parsedVisits = 0;
            if (parsedSpent  < 0) parsedSpent  = 0;
            if (parsedFb < 0 || parsedFb > 5) parsedFb = 0;
            Customer c(parsedId, name, phone, user, pass);
            c.setVisits(parsedVisits); c.setTotalSpent(parsedSpent); c.setFeedback(parsedFb);
            customers[count] = c; passwords[count] = pass; count++;
        }
        file.close();
        return count;
    }

    // Appends a completed order to orders.txt and also logs it to orders_log.txt.
    // Both files store a checksum so records can be verified later.
    static void saveOrderHistory(const Order& order) {
        ofstream file(DATA_FOLDER + "orders.txt", ios::app);
        if (file.is_open()) {
            long long cs = computeChecksum(order.getOrderId(), order.getTotal(), order.getTableNumber());
            file << order.getOrderId() << "|" << order.getCustomerName() << "|"
                 << order.getTableNumber() << "|" << order.getTotal() << "|"
                 << order.getOrderDate() << "|" << cs << "\n";
            file.close();
        }
        ofstream log(DATA_FOLDER + "orders_log.txt", ios::app);
        if (log.is_open()) {
            long long cs = computeChecksum(order.getOrderId(), order.getTotal(), order.getTableNumber());
            log << order.getOrderId() << "|" << order.getTotal() << "|"
                << order.getTableNumber() << "|" << order.getOrderDate() << "|" << cs << "\n";
            log.close();
        }
    }

    // Reads integrity records from orders_log.txt for use in sales verification
    static int loadIntegrityLog(IntegrityRecord records[], int maxCount) {
        int count = 0;
        ifstream file(DATA_FOLDER + "orders_log.txt");
        if (!file.is_open()) return 0;
        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, amount, tableNum, date, cs;
            getline(ss, id, '|'); getline(ss, amount, '|'); getline(ss, tableNum, '|');
            getline(ss, date, '|'); getline(ss, cs, '|');
            if (id.empty() || amount.empty()) continue;
            int parsedId, parsedTable; float parsedAmount; long long parsedCs;
            if (!safeStoi(id, parsedId))         continue;
            if (!safeStof(amount, parsedAmount)) continue;
            if (!safeStoi(tableNum, parsedTable)) parsedTable = 0;
            try { parsedCs = stoll(cs); } catch (...) { parsedCs = 0; }
            IntegrityRecord r;
            r.setOrderId(parsedId); r.setAmount(parsedAmount);
            r.setTableNum(parsedTable); r.setDate(date); r.setChecksum(parsedCs);
            records[count++] = r;
        }
        file.close();
        return count;
    }

    // Reads today's orders from both files, cross-checks checksums and log entries.
    // Returns the verified total; also sets rawTotal, corruptCount, and missingCount.
    static float loadTodaysSalesVerified(float& rawTotal, int& corruptCount, int& missingCount) {
        float verifiedTotal = 0.0f;
        rawTotal = 0.0f; corruptCount = 0; missingCount = 0;
        string today = getTodayDate();

        IntegrityRecord logRecords[MAX_ORDERS];
        int logCount = loadIntegrityLog(logRecords, MAX_ORDERS);
        bool foundInOrders[MAX_ORDERS] = {};

        ifstream file(DATA_FOLDER + "orders.txt");
        int seenIds[MAX_ORDERS]; int seenCount = 0;

        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                if (line.empty()) continue;
                stringstream ss(line);
                string id, cust, tableNum, amount, date, cs;
                getline(ss, id, '|'); getline(ss, cust, '|'); getline(ss, tableNum, '|');
                getline(ss, amount, '|'); getline(ss, date, '|'); getline(ss, cs, '|');

                if (id.empty() || amount.empty() || date.empty()) { corruptCount++; continue; }
                if (!isValidDate(date)) { corruptCount++; continue; }
                if (date != today) continue;

                int parsedId, parsedTable; float parsedAmount; long long parsedCs;
                if (!safeStoi(id, parsedId))         { corruptCount++; continue; }
                if (!safeStof(amount, parsedAmount)) { corruptCount++; continue; }
                if (!safeStoi(tableNum, parsedTable)) parsedTable = 0;
                try { parsedCs = stoll(cs); } catch (...) { parsedCs = 0; }

                if (parsedAmount <= 0.0f || parsedAmount > 500000.0f) { corruptCount++; continue; }

                // Reject duplicate order IDs in the same file
                bool isDuplicate = false;
                for (int i = 0; i < seenCount; i++)
                    if (seenIds[i] == parsedId) { isDuplicate = true; break; }
                if (isDuplicate) { corruptCount++; continue; }
                if (seenCount < MAX_ORDERS) seenIds[seenCount++] = parsedId;

                // Verify the stored checksum matches what we compute from the data
                long long expectedCs = computeChecksum(parsedId, parsedAmount, parsedTable);
                if (parsedCs != expectedCs) { corruptCount++; continue; }

                // Cross-reference against the integrity log; amounts must match
                bool inLog = false;
                for (int i = 0; i < logCount; i++) {
                    if (logRecords[i].getOrderId() == parsedId) {
                        inLog = true; foundInOrders[i] = true;
                        if (fabs(logRecords[i].getAmount() - parsedAmount) > 0.01f)
                            { corruptCount++; inLog = false; }
                        break;
                    }
                }
                if (!inLog) { corruptCount++; continue; }

                rawTotal += parsedAmount;
                verifiedTotal += parsedAmount;
            }
            file.close();
        }

        // Count log entries for today that have no matching record in orders.txt
        for (int i = 0; i < logCount; i++)
            if (!foundInOrders[i] && logRecords[i].getDate() == today)
                missingCount++;

        return verifiedTotal;
    }

    // Counts how many valid paid orders exist in orders.txt for today
    static int loadTodaysOrderCount() {
        int count = 0;
        ifstream file(DATA_FOLDER + "orders.txt");
        if (!file.is_open()) return 0;
        string today = getTodayDate(); string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, cust, table, amount, date;
            getline(ss, id, '|'); getline(ss, cust, '|'); getline(ss, table, '|');
            getline(ss, amount, '|'); getline(ss, date, '|');
            if (date != today || id.empty()) continue;
            int parsedId; float parsedAmt;
            if (!safeStoi(id, parsedId))     continue;
            if (!safeStof(amount, parsedAmt)) continue;
            if (parsedAmt <= 0) continue;
            count++;
        }
        file.close();
        return count;
    }

    // Writes only reserved tables to reservations.txt
    static void saveReservations(Table tables[], int count) {
        ofstream file(DATA_FOLDER + "reservations.txt");
        if (!file.is_open()) return;
        for (int i = 0; i < count; i++)
            if (tables[i].isReserved())
                file << tables[i].getTableNum() << "|"
                    << tables[i].getReservedFor() << "|"
                    << tables[i].getReservationTime() << "\n";
        file.close();
    }

    // Reads reservations and applies them to the matching Table objects
    static void loadReservations(Table tables[], int count) {
        ifstream file(DATA_FOLDER + "reservations.txt");
        if (!file.is_open()) return;
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string tableNum, name, time;
            getline(ss, tableNum, '|'); getline(ss, name, '|'); getline(ss, time, '|');
            if (tableNum.empty() || name.empty() || time.empty()) continue;
            int tNum;
            if (!safeStoi(tableNum, tNum))  continue;
            if (tNum < 1 || tNum > count)    continue;
            if (!isValidTimeSlot(time))      continue;
            if (name.size() < 2)            continue;
            for (int i = 0; i < count; i++)
                if (tables[i].getTableNum() == tNum) { tables[i].reserve(name, time); break; }
        }
        file.close();
    }

    // Overwrites feedbacks.txt with all current feedback records
    static void saveFeedbacks(FeedbackRecord feedbacks[], int count) {
        ofstream file(DATA_FOLDER + "feedbacks.txt");
        if (!file.is_open()) return;
        for (int i = 0; i < count; i++)
            file << feedbacks[i].getCustomerName() << "|"
            << feedbacks[i].getRating() << "|"
            << feedbacks[i].getComment() << "|"
            << feedbacks[i].getDate() << "\n";
        file.close();
    }

    // Reads feedback records; skips entries with invalid ratings or short comments
    static int loadFeedbacks(FeedbackRecord feedbacks[], int maxCount) {
        int count = 0;
        ifstream file(DATA_FOLDER + "feedbacks.txt");
        if (!file.is_open()) return 0;
        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            stringstream ss(line);
            string name, rating, comment, date;
            getline(ss, name, '|'); getline(ss, rating, '|');
            getline(ss, comment, '|'); getline(ss, date, '|');
            if (name.empty() || rating.empty() || name.size() < 2 || comment.size() < 3) continue;
            int parsedRating;
            if (!safeStoi(rating, parsedRating)) continue;
            if (parsedRating < 1 || parsedRating > 5) continue;
            FeedbackRecord fb(name, parsedRating, comment);
            fb.setDate(isValidDate(date) ? date : getTodayDate());
            feedbacks[count++] = fb;
        }
        file.close();
        return count;
    }

    // Writes the current waiting queue to queue.txt
    static void saveQueue(WaitingEntry queue[], int count) {
        ofstream file(DATA_FOLDER + "queue.txt");
        if (!file.is_open()) return;
        for (int i = 0; i < count; i++)
            file << queue[i].getName() << "|"
            << queue[i].getPartySize() << "|"
            << queue[i].getArrivalTime() << "\n";
        file.close();
    }

    // Reads the waiting queue; skips entries with invalid party sizes
    static int loadQueue(WaitingEntry queue[], int maxCount) {
        int count = 0;
        ifstream file(DATA_FOLDER + "queue.txt");
        if (!file.is_open()) return 0;
        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            stringstream ss(line);
            string name, size, time;
            getline(ss, name, '|'); getline(ss, size, '|'); getline(ss, time, '|');
            if (name.empty() || size.empty() || time.empty()) continue;
            int ps;
            if (!safeStoi(size, ps)) continue;
            if (ps < 1 || ps > 20)  continue;
            queue[count++] = WaitingEntry(name, ps, time);
        }
        file.close();
        return count;
    }

    // Persists the next order ID so it survives application restarts
    static void saveNextOrderId(int id) {
        ofstream f(DATA_FOLDER + "next_order_id.txt");
        if (f.is_open()) { f << id; f.close(); }
    }
    static int loadNextOrderId() {
        ifstream f(DATA_FOLDER + "next_order_id.txt");
        int id = 1;
        if (f.is_open()) { f >> id; f.close(); }
        return id;
    }
};
