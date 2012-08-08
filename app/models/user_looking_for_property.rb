class UserLookingForProperty < ActiveRecord::Base
  scope :select_for_search, select('distinct on (user_looking_for_properties.location, user_looking_for_properties.property_id,user_looking_for_properties.looking_for_id) user_looking_for_properties.id,user_looking_for_properties.location,user_looking_for_properties.property_id,user_looking_for_properties.looking_for_id')
  attr_accessible :looking_for_id, :latitude, :location, :longitude, :user_id , :property_id, :cart_id

  cattr_reader :per_page
  @@per_page = 3

  belongs_to :user
  belongs_to :property
  belongs_to :looking_for

  geocoded_by :location

  validates :looking_for_id , :presence => true
  validates :location, :length => {:minimum => 3,:maximum => 150},:presence => true
  validates :property_id , :presence => true

  class << self
    def search(prop_type,looking_for,location)
      joins(:property,:looking_for).where("(properties.id = :prop_type OR looking_fors.id = :looking_for OR user_looking_for_properties.location = :location)",:prop_type => prop_type,:looking_for=> looking_for,:location => location)
    end
  end

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

end
