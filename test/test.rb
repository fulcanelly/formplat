require 'socket'
require 'colored'


def checktime
  start = Time.now
  yield
  diff =  Time.now   - start
  puts "took secs #{diff.to_s.red}"
end

  
@s = TCPSocket.new('192.168.1.5', 80, connect_timeout: 5) 

module Proto 
  def mem 
    "freemem"
  end

  def led 
    "toggle"
  end

  def any 
    rand 
  end
end


extend Proto 

def exec_p(meth)
  puts meth.green
  checktime do 
    @s.puts meth 
    p @s.gets
  end
end

exec_p mem 
exec_p led
