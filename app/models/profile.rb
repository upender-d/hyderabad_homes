class Profile < ActiveRecord::Base
  attr_accessible :alternate_number, :dob, :employer, :first_name, :gender, :last_name, :latitude, :longitude, :mobile_number, :user_id, :work_location

  validates :first_name, :format => {:with => /^[a-zA-Z.\s]*$/i}, :length => {:minimum => 1, :maximum => 20}, :presence => true
  validates :last_name, :presence => true
  validates :mobile_number, :presence => true
  validates :work_location,  :presence => true
  validates :employer , :presence => true


  belongs_to :user

  acts_as_gmappable

  def gmaps4rails_address
    work_location
  end
  def gmaps4rails_infowindow
    "<h4>#{first_name}</h4>" << "<h4>#{work_location}</h4>"
  end


end
