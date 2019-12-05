import java.util.*;
import java.io.*;

/** class in java file must match filename, much sads. */
public class Arbitrage {
	/** helper lambda interface to let us filter */
	public interface FILTER<T> { boolean keep(T it); }
	/** Helper filter method */
	public static <T> List<T> filter(List<T> source, FILTER<T> fil) {
		List<T> match = new ArrayList<T>();
		for (T it : source) {
			if (it != null && fil.keep(it)) { match.add(it); }
		}
		return match;
	}
	
	/** WorkData class, used to hold information about where we are in the graph.  */
	public static class WorkData {
		/** Current location */
		public String at;
		/** Starting point */
		public String start;
		/** Path taken to get to 'at' */
		public List<String> path;
		/** Current rate */
		public double rate;
		/** Current score (rate - 1.0) / numtrades */
		public double score;
		@Override public String toString() {
			return "{ path: " + String.join(" => ", path) + " rate: " + rate + " score: " + score + "}";
		}
	}
	
	/** Transition function  */
	public static WorkData step(WorkData data, String place) {
		if (!data.at.equals(place)) {
			WorkData next = new WorkData();
			
			next.at = place;
			next.start = data.start;
			next.rate = data.rate * graph.get(data.at).get(place);
			next.score = (next.rate - 1.0) / data.path.size();
			next.path = new ArrayList<>(data.path);
			next.path.add(place);
			
			return next;
		}	
		return data;
	}
	/** Helper to transition a node back to start  */
	public static  WorkData stepToStart(WorkData data) { return step(data, data.start); }
	
	// data structures for working 
	/** Graph of VertNames => { Transitions To Other Verts } */
	private static final Map<String, Map<String, Double>> graph = new HashMap<>();
	/** Queue of work to do. */
	private static final LinkedList<WorkData> queue = new LinkedList<>();
	/** Profitable paths */
	private static final List<WorkData> profitable = new ArrayList<>();
	
	public static void main(String[] args) {
		try { // Checked exceptions are annoying 
			// Open the file
			Scanner file = new Scanner(new File("stuff.dat"));
			
			// Read entire file
			while (file.hasNext()) {
				// One line at a time ez
				String line = file.nextLine();
				// Unfortunately java is pretty clumsy even with a bit of assistance
				List<String> parts = filter(Arrays.asList(line.split("\t")), it -> !"".equals(it));
				// Skip lines that don't have exactly 3 things on them
				if (parts.size() != 3) { continue; }
				
				// From To Rate#
				String from = parts.get(0);
				String to = parts.get(1);
				String ratestr = parts.get(2);
				double rate = 0;
				// Skip any triplets without a number as the third thing
				try {rate = Double.parseDouble(ratestr); }
				catch (Exception e) { continue; }
				
				// If we don't have transitions for either, create them
				if (!graph.containsKey(from)) { graph.put(from, new HashMap<>()); }
				if (!graph.containsKey(to)) { graph.put(to, new HashMap<>()); }
				
				// Record transition rates into graph 
				graph.get(from).put(to, rate);
				graph.get(to).put(from, 1.0 / rate);
			} file.close(); // Done loading file. Thanks javalint for the hint.
			
			//System.out.println("Size: " + graph.size());
			// Iterate our graph and seed our queue with starting everywhere.
			for (String key : graph.keySet()) {
				WorkData data = new WorkData();
				data.at = key;
				data.start = key;
				data.rate = 1.0;
				data.score = 0.0;
				data.path = Arrays.asList(key);
				
				queue.add(data);
			}
			
			long start = System.nanoTime();
			// Work while there's work to do.
			while (queue.size() > 0) {
				WorkData current = queue.poll();
				WorkData compare = step(current, current.start);
				
				for (String key : graph.keySet()) {
					// Skip anywhere we've already been
					if (!current.path.contains(key)) {
						
						WorkData next = step(current, key);
						WorkData scored = step(next, next.start);
						
						if (scored.rate > 1) {
							profitable.add(scored);
						}
						
						if (scored.rate >= compare.rate) {
							queue.add(next);
						}
						
					}
					
				} // End graphloop
			} // End workloop
			
			long end = System.nanoTime();
			double diff = (end-start) / 1000000.0; // ns -> ms
			
			// Efficiently build output before dumping to file:
			StringBuilder str = new StringBuilder();
			// Sort ascending by rate
			Collections.sort(profitable, (a,b) -> Double.compare(a.rate, b.rate) );
			
			// Iterate our profitable stuff and add to string builder.
			for (int i = 0; i < profitable.size(); i++) {
				WorkData data = profitable.get(i);
				// Turns out it's alright to do one big ++++++++ each loop body and append to strbuilder
				// Instead of a ton of smaller appends
				// Java will still compile this to a fairly efficient String.join
				str.append("Path " + i + ": " + String.join(" => ", data.path) + ", $1000.00 => $" + data.rate * 1000.0 +"\n");
			}
			
			System.out.println("Done! It took " + diff + "ms");
			str.append("Done! It took " + diff + "ms\n");
			
			System.out.println("Found " + profitable.size() + " profitable paths");
			str.append("Found " + profitable.size() + " profitable paths\n");
			
			// Sort Descending by rate (note order is reversed from the first sort...)
			Collections.sort(profitable, (a,b) -> Double.compare(b.rate, a.rate));
			System.out.println("\n\nBest path without repeating: " + profitable.get(0));
			str.append("\n\nBest path without repeating: " + profitable.get(0) + "\n");
			
			// Sort Descending by score (note order is reversed from the first sort...)
			Collections.sort(profitable, (a,b) -> Double.compare(b.score, a.score));
			System.out.println("\n\nMost efficient path: " + profitable.get(0));
			str.append("\n\nMost efficient path: " + profitable.get(0) + "\n");
			
			PrintWriter out = new PrintWriter(new File("outputjava.txt"));
			out.println(str.toString());
			out.flush();
			out.close();
			
			
		} catch (Exception e) { throw new RuntimeException(e); } // Checked exceptions are annoying 
	} // End main
} // End class 