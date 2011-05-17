
# dan: 
# this file contains two classes, 'TimeAwareSet' and 'TimeAwareHash'
# that implement "forgetful" versions of the normal python Set and Hash
# objects.  Basically, you create each data-structure by specifying a
# timeout, and the data-structure will forget anything you put in it 
# older than that value (with a time error controlled by the 'granularity'
# parameter).  It uses a bucketized approach that seems more efficient than
# linearly cycling through timestamped entries to clean them out.  However,
# we have not really tested this to show that it is truly better.  



class TimeAwareSet:

  def __init__(self, timeout_len, granularity): 
    self.timeout_len = timeout_len
    self.granularity = granularity
    self.buckets = [] 
    num_buckets = timeout_len / granularity; 
    for i in range(0,num_buckets): 
      self.buckets.append(set())

    self.last_ts = 0 

  def update_bucket_state(self, cur_time):
    
    if(self.last_ts == 0):
      self.last_ts = cur_time
      return 

 #   print "cur = " + str(cur_time) + " last_ts = " + str(self.last_ts)
    time_diff = cur_time - self.last_ts
    num_expired_buckets = time_diff / self.granularity

    if num_expired_buckets == 0: 
      return 

    if(num_expired_buckets > len(self.buckets)):
        num_expired_buckets = len(self.buckets)
    
    for i in range(0,num_expired_buckets):
#      print "expiring bucket: " + str(self.buckets[len(self.buckets) - 1])
      del self.buckets[len(self.buckets) - 1]
      self.buckets.insert(0, set()) 

    self.last_ts = cur_time

  def insert(self, item, cur_time): 

    self.update_bucket_state(cur_time)
    
    # skip first bucket
    for i in range(1, len(self.buckets)): 
      if item in self.buckets[i]:
        self.buckets[i].remove(item) 

    self.buckets[0].add(item)
    
  def is_member(self, item, cur_time): 

    self.update_bucket_state(cur_time)

    for i in range(0,len(self.buckets)):
      if item in self.buckets[i]:
        return True

    return False

  def remove(self, item): 
    for i in range(0,len(self.buckets)): 
      if item in self.buckets[i]:
        self.buckets[i].remove(item) 
    

  def __len__ (self): 
    sum = 0
    for s in self.buckets:
      sum = sum + len(s) 
    return sum

  def __str__(self):
    master_set = set()
    for s in self.buckets:
      master_set = master_set | s
    return str(master_set) 


class TimeAwareHash:

  def __init__(self, timeout_len, granularity): 
    self.timeout_len = timeout_len
    self.granularity = granularity
    self.buckets = [] 
    num_buckets = timeout_len / granularity; 
    for i in range(0,num_buckets): 
      self.buckets.append({})

    self.last_ts = 0 

  def update_bucket_state(self, cur_time):
    if(self.last_ts == 0):
      self.last_ts = cur_time

    time_diff = cur_time - self.last_ts
    num_expired_buckets = time_diff / self.granularity

    if num_expired_buckets == 0: 
      return 

    if(num_expired_buckets > len(self.buckets)):
        num_expired_buckets = len(self.buckets)
    for i in range(0,num_expired_buckets):
      del self.buckets[len(self.buckets) - 1]
      self.buckets.insert(0, {}) 

    self.last_ts = cur_time

  def insert(self, key, value, cur_time): 

    self.update_bucket_state(cur_time)

    # skip first bucket
    for i in range(1,len(self.buckets)): 
      if key in self.buckets[i]:
        del self.buckets[i][key]  

    self.buckets[0][key] = value

  def lookup(self, key, cur_time):

    self.update_bucket_state(cur_time)

    for i in range(0,len(self.buckets)):
      if key in self.buckets[i]:
        return self.buckets[i][key]

    return None

  def remove(self,key):
    for i in range(0,len(self.buckets)): 
      if key in self.buckets[i]:
        del self.buckets[i][key]  

  def __delitem__(self,key):
    self.remove(key)

  def __len__ (self): 
    sum = 0
    for s in self.buckets:
      sum = sum + len(s) 
    return sum 
  
  def __str__(self):
    master_hash = {} 
    for h in self.buckets:
      master_hash.update(h)
    return str(master_hash) 

# simple test, to be verified visually 

if __name__ == '__main__': 

  t = TimeAwareSet(2,1)
  print t
  for i in range(1,10):
    t.insert(i,i)
    print t
    if i == 3:
      t.remove(2)
      print "deleted 2"
      print t

  print "2 is a member: " + str(t.is_member(2,9))
  print "9 is a member: " + str(t.is_member(9,9))

  h = TimeAwareHash(2,1)
  print h
  for i in range(1,10):
    h.insert(i, str(i), i)
    print h
    if i == 3:
      del h[2]

  print "value for '2': " + str(h.lookup(2,9))
  print "value for '9': " + str(h.lookup(9,9))


