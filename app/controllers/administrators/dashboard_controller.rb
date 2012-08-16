class Administrators::DashboardController < ApplicationController
  def index
    @user_properties = UserProperty.all
    @user_looking_for_properties = UserLookingForProperty.all
  end

  def users_list
    @users = User.all
  end

end
