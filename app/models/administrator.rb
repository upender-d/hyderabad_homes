class Administrator < ActiveRecord::Base
  # Include default devise modules. Others available are:
  # :token_authenticatable, :encryptable, :confirmable, :lockable, :timeoutable and :omniauthable
  devise :database_authenticatable, :registerable,
         :recoverable, :rememberable, :trackable, :validatable

  # Setup accessible (or protected) attributes for your model
  attr_accessible :email, :password, :role, :first_name, :last_name, :mobile_number, :status, :password_confirmation, :remember_me
  # attr_accessible :title, :body

  #validations

  validates :first_name , :presence => true
  validates :last_name , :presence => true
  validates :mobile_number , :presence => true


end
