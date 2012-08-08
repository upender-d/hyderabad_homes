class Users::UserPropertiesController < ApplicationController

  before_filter :authenticate_user!

  def index

    if params[:addr].present?

      @user_properties = UserProperty.near(params[:addr], 50, :order => :distance).where(:user_id=> current_user.id).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
      @json = UserProperty.near(params[:addr], 50, :order => :distance).where(:user_id=> current_user.id).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC").to_gmaps4rails
    else
      @user_properties = UserProperty.where(:user_id => current_user.id).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
      @json = UserProperty.where(:user_id=>current_user.id).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC").to_gmaps4rails
    end
  end

  def new
    @user_property = UserProperty.new
  end

  def create
    @user_property = UserProperty.new(params[:user_property])
    @user_property.user_id = current_user.id
    @user_property.location = params[:addr]
    if @user_property.save
      redirect_to [:users, @user_property]
      flash[:notice] = "Property Created successfully"
    end
  end

  def show
    @user_property = UserProperty.find(params[:id])
    @json = UserProperty.where(:id => @user_property.id).to_gmaps4rails
  end

  def edit
    @user_property = UserProperty.find(params[:id])
  end

  def update
    @user_property = UserProperty.find(params[:id])
    @user_property.user_id = current_user.id

    if  @user_property.update_attributes(params[:user_property])
      redirect_to([:users,@user_property])
      flash[:notice] = "Profile Updated Successfully."
    else
      render :action => "edit"
    end
  end

  def destroy
    @user_property = UserProperty.find(params[:id])
    @user_property.destroy
    redirect_to users_user_properties_path
  end

  def search_properties_user
    @search_properties = UserProperty.search_properties(params[:addr],params[:search][:ownership_type_id],params[:search][:property_id])
    @json = UserProperty.search_properties(params[:addr],params[:search][:ownership_type_id],params[:search][:property_id]).to_gmaps4rails
  end

end
