class Users::UserLookingForPropertiesController < ApplicationController

  before_filter :authenticate_user!

  def index

    if params[:addr].present?
      @user_looking_for_properties = UserLookingForProperty.near(params[:addr], 50, :order => :distance).where(:user_id => current_user.id).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
      @json = UserLookingForProperty.near(params[:addr], 50, :order => :distance).where(:user_id => current_user.id).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC").to_gmaps4rails
    else
      @user_looking_for_properties = UserLookingForProperty.where(:user_id=>current_user.id).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
      @json = UserLookingForProperty.where(:user_id=>current_user.id).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC").to_gmaps4rails
    end
  end

  def new
    @user_looking_for_property = UserLookingForProperty.new
  end

  def create
    @user_looking_for_property = UserLookingForProperty.new(params[:user_looking_for_property])
    @user_looking_for_property.user_id = current_user.id
    @user_looking_for_property.location = params[:addr]
    if @user_looking_for_property.save
      redirect_to [:users, @user_looking_for_property]
      flash[:notice] = "Property Created successfully"
    end
  end

  def show
    @user_looking_for_property = UserLookingForProperty.find(params[:id])
    @json = UserLookingForProperty.all.to_gmaps4rails
  end

  def edit
    @user_looking_for_property = UserLookingForProperty.find(params[:id])
  end

  def update
    @user_looking_for_property = UserLookingForProperty.find(params[:id])
    @user_looking_for_property.user_id = current_user.id

    if  @user_looking_for_property.update_attributes(params[:user_looking_for_property])
      redirect_to([:users,@user_looking_for_property])
      flash[:notice] = "Profile Updated Successfully."
    else
      render :action => "edit"
    end
  end

  def destroy
    @user_looking_for_property = UserLookingForProperty.find(params[:id])
    @user_looking_for_property.destroy
    redirect_to users_user_looking_for_properties_path
  end
end
