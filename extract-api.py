import sys

if __name__ == "__main__":
  api = open(sys.argv[1])
  full = open(sys.argv[2])
  pruned = open(sys.argv[3], 'w')
  function_name = 0
  slot = 1
  fixed = 2
  sentinel = 3
  symbolic = 4
  
  apis = []
  fulls = []
  for i in api:
    apis.append(i.strip())
  for i in full:
    fulls.append(i.strip())
  i = 0
  j = 0
  while i < len(apis) and j < len(fulls):
    current_api = apis[i].split(",")
    current_full = fulls[j].split(",")
    while current_api[function_name] != current_full[function_name] or current_api[slot] != current_full[slot]:
      if current_api[function_name] != current_full[function_name]:
        if current_api[function_name] < current_full[function_name]:
          i += 1
        else:
          j += 1
      elif current_api[slot] < current_full[slot]:
        i += 1
      else:
        j += 1
      if j >= len(fulls) or i >= len(apis):
        break
      current_full = fulls[j].split(",")
      current_api = apis[i].split(",")
    if j >= len(fulls) or i >= len(apis):
      break
    assert (current_api[function_name] == current_full[function_name])
    assert (current_api[slot] == current_full[slot])
    pruned.write(fulls[j] + "\n")
    j += 1  
    i += 1