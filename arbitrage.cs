using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Arbitrage {
	
	class WorkData {
		public string start;
		public string at;
		public List<string> path;
		public double rate;
		public double score;

		public override string ToString() {
			StringBuilder str = new StringBuilder();
			str.Append("Start: ");
			str.Append(start);
			str.Append("\nPath: [");
			foreach (var p in path) { 
				str.Append(p);
				str.Append(", ");
			}
			str.Append("]\nRate: ");
			str.Append(rate);
			str.Append("\nScore: ");
			str.Append(score);


			return str.ToString();
		}

		public string ShortString() {
			return $"Path: [ {string.Join(" -> ", path)} ] $1,000 => ${rate*1000.0:#,##0.00}";
		}
	}
	class Graph : Dictionary<string, Dictionary<string, double>> {
		/// <summary> Transition function through the graph </summary>
		/// <param name="data"> Current graph state to transition </param>
		/// <param name="place"> New vertex to transition to </param>
		/// <returns> Results of transition </returns>
		public WorkData Step(WorkData data, string place) {

			// If data is not already at the place we want to transition it to
			if (data.at != place) {
				WorkData next = new WorkData();
				// Started at the same place
				next.start = data.start;
				// Now at the new place
				next.at = place;
				// Apply the rate 
				next.rate = data.rate * this[data.at][place];
				// Score the node based on gain per trades
				next.score = (next.rate - 1.0) / data.path.Count;
				// Copy all elements from the previous path into the new one
				next.path = new List<string>(data.path);
				// Then add the place it is at now
				next.path.Add(place);

				return next;
			}
			// Already there? Return what we were given
			return data;
		}
	}

	class Program {

		public static string SourceFileDirectory([CallerFilePath] string callerPath = "[NO PATH]") {
			return callerPath.Substring(0, callerPath.Replace('\\', '/').LastIndexOf('/'));
		}

		static void Main(string[] args) {
			Console.WriteLine(SourceFileDirectory());
			string text = File.ReadAllText(SourceFileDirectory() + "/stuff.dat");
			Console.WriteLine(text);
			string[] lines = text.Split('\n');

			Graph graph = new Graph();
			Queue<WorkData> queue = new Queue<WorkData>();
			List<WorkData> finished = new List<WorkData>();

			// for (String line : lines) // java version
			foreach (string line in lines) {
				// Split
				string[] parts = line.Split(' ', '\t').Where(it => (it != "")).ToArray();

				Console.WriteLine(parts.Length + " " + line);
				if (parts.Length != 3) { continue; }
				string from = parts[0];
				string to = parts[1];
				double rate;
				if (!double.TryParse(parts[2], out rate)) { continue; }

				// Add new verticies if they don't already exist
				if (!graph.ContainsKey(from)) { 
					graph[from] = new Dictionary<string, double>();
				}
				if (!graph.ContainsKey(to)) {
					graph[to] = new Dictionary<string, double>();
				}

				// Set transition rate into graph
				graph[from][to] = rate;
				graph[to][from] = 1.0 / rate;
			}
			Console.WriteLine("Test");

			foreach (var pair in graph) {
				Console.WriteLine(pair.Key + " : {");
				foreach (var pair2 in pair.Value) {
					Console.WriteLine("\t" + pair2.Key + " : " + pair2.Value);
				}
				Console.WriteLine("}");

				WorkData data = new WorkData();
				data.start = pair.Key;
				data.at = pair.Key;
				data.rate = 1.0;
				data.score = 0.0;
				data.path = new List<string>();
				data.path.Add(pair.Key);

				queue.Enqueue(data);
			}

			DateTime start = DateTime.UtcNow;
			while (queue.Count > 0) {
				WorkData data = queue.Dequeue();
				

				WorkData compare = graph.Step(data, data.start);

				foreach (var pair in graph) {

					// If we haven't been there yet
					if (!data.path.Contains(pair.Key)) {
						WorkData next = graph.Step(data, pair.Key);
						WorkData final = graph.Step(next, data.start);

						// See if it is profitable, then add it to the list of profitable trades
						if (final.rate > 1) {
							finished.Add(final);
						}

						// Ignore anywhere we lose money!
						if (final.rate >= compare.rate) {
							queue.Enqueue(next);
						}

					}

				}

			}
			DateTime end = DateTime.UtcNow;
			TimeSpan diff = end - start;

			finished.Sort((a, b) => (a.rate - b.rate) < 0 ? -1 : 1 );
			StringBuilder output = new StringBuilder();

			foreach (var profit in finished) {
				Console.WriteLine(profit.ShortString());
				output.Append(profit.ShortString());
				output.Append('\n');
			}


			Console.WriteLine("\n\nFinished in " + diff.TotalMilliseconds + "ms");
			output.Append("\n\nFinished in " + diff.TotalMilliseconds + "ms");

			finished.Sort((a, b) => (a.rate - b.rate) > 0 ? -1 : 1 );
			Console.WriteLine("\n\nBest Overall Arbitrage: \n" + finished[0]);
			output.Append("\n\nBest Overall Arbitrage: \n" + finished[0]);

			finished.Sort((a, b) => (a.score - b.score) > 0 ? -1 : 1 );
			Console.WriteLine("\n\nMost Efficient Arbitrage: \n" + finished[0]);
			output.Append("\n\nMost Efficient Arbitrage: \n" + finished[0]);

			File.WriteAllText(SourceFileDirectory()+"/output.txt", output.ToString());

			Console.Read();
		}
	}
}
