# app.py
# Owner: Ansh
# Flask dashboard skeleton. In Phase 1 this serves placeholder/dummy data.
# In Phase 2, replace get_dummy_data() with real queries against MySQL
# (resource_logs, scheduling_results, energy_estimates tables).

from flask import Flask, jsonify, render_template_string

app = Flask(__name__)

# ---------- Placeholder data (Phase 1 only) ----------
def get_dummy_data():
    return {
        "timestamps": ["10:00", "10:01", "10:02", "10:03", "10:04"],
        "cpu_usage": [12, 18, 45, 30, 22],
        "mem_usage": [34, 35, 36, 38, 37],
        "energy_by_algorithm": {
            "FCFS": 46.3,
            "SJF": 39.3,
            "RoundRobin": 51.1,
            "Priority": 42.8,
            "EnergyAware": 33.5
        }
    }

# ---------- API endpoint the dashboard's JS will call ----------
@app.route("/api/dashboard-data")
def dashboard_data():
    return jsonify(get_dummy_data())

# ---------- Simple HTML page with Chart.js placeholders ----------
DASHBOARD_HTML = """
<!DOCTYPE html>
<html>
<head>
  <title>Energy-Aware System Dashboard (Phase 1 skeleton)</title>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.0/chart.umd.min.js"></script>
</head>
<body style="font-family: sans-serif; max-width: 900px; margin: 40px auto;">
  <h1>Resource & Energy Dashboard</h1>
  <p>Phase 1 skeleton — currently showing placeholder data. Will connect to MySQL in Phase 2.</p>

  <h3>CPU / Memory Usage Over Time</h3>
  <canvas id="usageChart" height="100"></canvas>

  <h3>Estimated Energy by Scheduling Algorithm</h3>
  <canvas id="energyChart" height="100"></canvas>

  <script>
    fetch('/api/dashboard-data')
      .then(res => res.json())
      .then(data => {
        new Chart(document.getElementById('usageChart'), {
          type: 'line',
          data: {
            labels: data.timestamps,
            datasets: [
              { label: 'CPU %', data: data.cpu_usage, borderColor: 'blue', fill: false },
              { label: 'Memory %', data: data.mem_usage, borderColor: 'green', fill: false }
            ]
          }
        });

        new Chart(document.getElementById('energyChart'), {
          type: 'bar',
          data: {
            labels: Object.keys(data.energy_by_algorithm),
            datasets: [{
              label: 'Estimated Energy (units)',
              data: Object.values(data.energy_by_algorithm),
              backgroundColor: ['#4c72b0','#55a868','#c44e52','#8172b2','#ccb974']
            }]
          }
        });
      });
  </script>
</body>
</html>
"""

@app.route("/")
def index():
    return render_template_string(DASHBOARD_HTML)

if __name__ == "__main__":
    print("=== Ansh: Dashboard Skeleton ===")
    print("Run this, then open http://localhost:5000 in a browser.")
    app.run(debug=True, port=5000)
