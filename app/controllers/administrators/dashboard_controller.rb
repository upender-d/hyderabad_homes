class Administrators::DashboardController < ApplicationController
  def index
    @users = User.all
    @user_properties = UserProperty.all
    @user_looking_for_properties = UserLookingForProperty.all
  end
end
