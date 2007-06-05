class Compare < String
	include Comparable
	def <=>(other)
		self.to_i <=> other.to_i
	end
	def to_i
		self.scan(/\d+/)[0].to_s.to_i
	end
end
