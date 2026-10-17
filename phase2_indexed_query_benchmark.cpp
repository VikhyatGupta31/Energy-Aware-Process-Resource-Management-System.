// indexed_query_benchmark.cpp
// Owner: Ruchi (Phase 2)
// Runs the same query with and without an index, times both using C++ chrono,
// and logs the comparison into queries_log — this is the actual evidence
// for your "indexing improves performance" claim in the final report.

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <chrono>
#include <iostream>
#include <memory>

using namespace std;
using namespace sql;
using namespace std::chrono;

unique_ptr<Connection> connectToDB() {
    mysql::MySQL_Driver* driver = mysql::get_mysql_driver_instance();
    unique_ptr<Connection> conn(driver->connect("tcp://127.0.0.1:3306", "root", "your_password"));
    conn->setSchema("energy_aware_system");
    return conn;
}

// Times a SELECT query and returns milliseconds elapsed.
double timeQuery(Connection* conn, const string& queryText) {
    unique_ptr<Statement> stmt(conn->createStatement());
    auto start = high_resolution_clock::now();
    unique_ptr<ResultSet> res(stmt->executeQuery(queryText));
    int rowCount = 0;
    while (res->next()) rowCount++; // force full result retrieval for a fair timing
    auto end = high_resolution_clock::now();
    return duration<double, milli>(end - start).count();
}

void logQueryResult(Connection* conn, const string& queryText, double ms, bool usedIndex) {
    unique_ptr<PreparedStatement> stmt(conn->prepareStatement(
        "INSERT INTO queries_log (query_text, execution_time_ms, used_index) VALUES (?, ?, ?)"));
    stmt->setString(1, queryText);
    stmt->setDouble(2, ms);
    stmt->setBoolean(3, usedIndex);
    stmt->execute();
}

int main() {
    try {
        auto conn = connectToDB();
        cout << "=== Ruchi: Indexed Query Benchmark ===\n\n";

        string testQuery = "SELECT * FROM processes WHERE burst_time > 4";

        // Step 1: drop the index, time the query (full table scan)
        unique_ptr<Statement> setup(conn->createStatement());
        try { setup->execute("DROP INDEX idx_burst_time ON processes"); } catch (...) {}

        double timeWithoutIndex = timeQuery(conn.get(), testQuery);
        logQueryResult(conn.get(), testQuery, timeWithoutIndex, false);
        cout << "Without index: " << timeWithoutIndex << " ms\n";

        // Step 2: add the index back, time the same query again
        setup->execute("CREATE INDEX idx_burst_time ON processes(burst_time)");
        double timeWithIndex = timeQuery(conn.get(), testQuery);
        logQueryResult(conn.get(), testQuery, timeWithIndex, true);
        cout << "With index:    " << timeWithIndex << " ms\n";

        cout << "\nBoth results logged to queries_log for the dashboard.\n";
        cout << "Note: on a small table like this test set, the difference may be\n"
             << "negligible or even reversed — that's expected and worth mentioning\n"
             << "in your report (see 'query processing' viva notes on optimizer behavior).\n";

    } catch (SQLException& e) {
        cerr << "MySQL error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
