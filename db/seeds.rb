# This file should contain all the record creation needed to seed the database with its default values.
# The data can then be loaded with the rake db:seed (or created alongside the db with db:setup).
#
# Examples:
#
#   cities = City.create([{ name: 'Chicago' }, { name: 'Copenhagen' }])
#   Mayor.create(name: 'Emanuel', city: cities.first)

#==========================User Looking For Properties ===============================

UserLookingForProperty.create(user_id: 1, location: 'Street Number 3, Krishna Nagar, Yousufguda, Hyderabad, Andhra Pradesh, India', :property_id=> 1, :looking_for_id => 1)

UserLookingForProperty.create(user_id: 2, location: 'Road Number 10, Nandi Nagar, Banjara Hills, Hyderabad, Andhra Pradesh, India', :property_id=> 2, :looking_for_id => 2)

UserLookingForProperty.create(user_id: 1, location: 'Hitech City Rd, Kavuri Hills, Madhapur, Hyderabad, Andhra Pradesh, India', :property_id=> 3, :looking_for_id => 3)
