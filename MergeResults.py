import sys

if __name__ == "__main__":
  to_write_results_file = open(sys.argv[1], 'w')
  results = ["name,symbolic-true-positive,symbolic-false-positive,fixed-true-positive,fixed-false-positive,sentinel-true-positive,sentinel-false-positive"]
  i = 2
  while i < len(sys.argv):
    results_file = open(sys.argv[i])
    results_file.readline()
    rates_results = results_file.readline()
    
    results.append(rates_results)
    
    i += 1
  for line in results:
    to_write_results_file.write(line + "\n")