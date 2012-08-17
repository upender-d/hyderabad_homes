class UserProperty < ActiveRecord::Base
  attr_accessible :is_current_location, :latitude, :location, :longitude, :ownership_type_id, :user_id, :property_id

  scope :select_for_search, select('distinct on (user_properties.location, user_properties.property_id,user_properties.ownership_type_id) user_properties.id,user_properties.property_id,user_properties.location , user_properties.ownership_type_id')

  @per_page = 3

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

  def self.search_properties(location,prop_type,ownership_type)
    joins(:property,:ownership_type).select_for_search.where("user_properties.location = :location OR properties.name = :prop_type OR ownership_types.name = :ownership_type",:location => location , :prop_type => prop_type,:ownership_type=> ownership_type)
  end


end
