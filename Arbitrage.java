import java.util.*;
import java.io.*;
import java.lang.reflect.*;

public class Arbitrage {
	public interface FILTER<T> { boolean keep(T it); }
	public static <T> List<T> filter(List<T> source, FILTER<T> fil) {
		List<T> match = new ArrayList<T>();
		
		for (int i = 0; i < source.size(); i++) {
			if (source.get(i) != null && fil.keep(source.get(i))) {
				match.add(source.get(i));
			}
		}
		
		return match;
	}
	
	public static class WorkData {
		public String at;
		public String start;
		public List<String> path;
		public double rate;
		public double score;
	}
	
	public static WorkData stepTo(WorkData data, String place) {
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
	
	private static final Map<String, Map<String, Double>> graph = new HashMap<>();
	public static void main(String[] args) {
		try {
			Scanner file = new Scanner(new File("stuff.dat"));
			
			LinkedList<WorkData> queue = new LinkedList<>();
			List<WorkData> profitable = new ArrayList<>();
			
			while (file.hasNext()) {
				String line = file.nextLine();
				List<String> parts = Arrays.asList(line.split("\t"));
				
				parts = filter(parts, it -> !"".equals(it) );
				
				// System.out.println(parts.size() + " : " + line);
				
				if (parts.size() != 3) { continue; }
				
				String from = parts.get(0);
				String to = parts.get(1);
				String ratestr = parts.get(2);
				double rate = 0;
				try {
					rate = Double.parseDouble(ratestr);
				} catch (Exception e) { continue; }
				
				if (!graph.containsKey(from)) {
					graph.put(from, new HashMap());
				}
				if (!graph.containsKey(to)) {
					graph.put(to, new HashMap());
				}
				
				graph.get(from).put(to, rate);
				graph.get(to).put(from, 1.0 / rate);
			}
			
			//System.out.println("Size: " + graph.size());
			for (String key : graph.keySet()) {
				WorkData data = new WorkData();
				data.at = key;
				data.start = key;
				data.rate = 1.0;
				data.score = 0.0;
				data.path = Arrays.asList(key);
				
				queue.add(data);
			}
			
			
			while (queue.size() > 0) {
				WorkData current = queue.poll();
				WorkData compare = stepTo(current, current.start);
				
				for (String key : graph.keySet()) {
					// Skip anywhere we've already been
					if (!current.path.contains(key)) {
						
						WorkData next = stepTo(current, key);
						WorkData scored = stepTo(next, next.start);
						
						if (scored.rate > 1) {
							profitable.add(scored);
						}
						
						if (scored.rate >= compare.rate) {
							queue.add(next);
						}
						
					}
					
				}
				
			}
			
			System.out.println("Done! Found " + profitable.size() + " profitable paths");
			
			StringBuilder str = new StringBuilder();
			
			// Sort ascending by rate
			Collections.sort(profitable, (a,b) -> Double.compare(a.rate, b.rate) );
			
			for (int i = 0; i < profitable.size(); i++) {
				str.append("Path ");
				str.append(i);
				str.append(": ");
				WorkData data = profitable.get(i);
				str.append(String.join(" => ", data.path));
				str.append(", $1000.00 => $");
				str.append(data.rate * 1000.0);
				str.append("\n");
			}
			
			
			PrintWriter out = new PrintWriter(new File("outputjava.txt"));
			out.println(str.toString());
			out.flush();
			out.close();
			
			
		} catch (Exception e) {
			System.out.println("Error: " + e);
		}
	}
	
}