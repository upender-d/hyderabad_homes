class User < ActiveRecord::Base
  has_one :profile,:dependent=>:destroy
  has_many :user_properties, :dependent => :destroy
  has_many :user_looking_for_properties, :dependent => :destroy
  has_many :user_contacts, :dependent => :destroy
  # Include default devise modules. Others available are:
  # :token_authenticatable, :encryptable,  :lockable, :timeoutable and :omniauthable,:confirmable
  devise :database_authenticatable, :registerable,
         :recoverable, :rememberable, :trackable, :validatable

  # Setup accessible (or protected) attributes for your model
  attr_accessible :email, :password, :password_confirmation, :remember_me
  # attr_accessible :title, :body
end
