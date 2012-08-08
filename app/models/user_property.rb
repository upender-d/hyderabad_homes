class UserProperty < ActiveRecord::Base
  attr_accessible :is_current_location, :latitude, :location, :longitude, :ownership_type_id, :user_id, :property_id

  cattr_reader :per_page
  @@per_page = 3

  belongs_to :user
  belongs_to :property
  belongs_to :ownership_type

  geocoded_by :location

  validates :ownership_type_id , :presence => true
  validates :location, :length => {:minimum => 3,:maximum => 150},:presence => true
  validates :property_id , :presence => true

  after_validation :geocode, :if => :location_changed?

  acts_as_gmappable

  def gmaps4rails_address
    location
  end
  def gmaps4rails_infowindow
    "<h4>#{location}</h4>"
  end

  def gmaps4rails_marker_picture
    {
        "picture" => "/assets/user.png",
        "width" => "30",
        "height" => "40",
    }
  end

  def self.search_properties(query1,query2,query3)
    joins(:property,:ownership_type).where("user_properties.location LIKE ? or  properties.name LIKE ? or ownership_types.name Like ?","%#{query1}%", "%#{query3}%", "%#{query2}%")
  end


end
