class OwnershipType < ActiveRecord::Base
  attr_accessible :name

  @per_page = 3

  has_many :user_properties

  validates_uniqueness_of :name , :case_sensitive => false

  def self.search_properties(query1)
    #where("lower(name) LIKE ? OR upper(name) LIKE ? ","%#{query1}%","%#{query2}%")
    where("lower(name) LIKE ? ","%#{query1.downcase}%")
  end

end
