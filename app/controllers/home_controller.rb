class HomeController < ApplicationController
  def index

  end

  def search_properties

    @search_properties = UserLookingForProperty.search(params[:search][:property_id],params[:search][:looking_for_id],params[:addr])
    puts "ddddddddddddddddddddddddd",@search_properties.count
    @json = UserLookingForProperty.search(params[:search][:property_id],params[:search][:looking_for_id],params[:addr]).to_gmaps4rails

# @carts = [Cart.new]
  end

  def add_to_cart
    if current_user.nil?
      deny_access
    else

=begin
 @user_looking_for_property = UserLookingForProperty.new(params[:cart])
      @user_looking_for_property.user_id = current_user.id
      @user_looking_for_property.location = params[:cart][:location]
      if @user_looking_for_property.save
        redirect_to users_user_looking_for_properties_path
        flash[:notice] = "User Looking for property saved successfully"
      end

=end
      redirect_to home_add_to_favourites_path
    end
  end

  def add_to_favourites
    @search_properties = UserLookingForProperty.search(session[:property],session[:looking_for],session[:location])
    @json = UserLookingForProperty.search(session[:property],session[:looking_for],session[:location]).to_gmaps4rails
  end

  def create_properties
    @var = UserLookingForProperty.find(params[:property])
    @var.each do |product|
      @var1  = UserLookingForProperty.new(:user_id => current_user.id, :property_id => product.property_id,:location => product.location,:looking_for_id => product.looking_for_id)
      @var1.save
    end
    redirect_to users_user_looking_for_properties_path , :notice => "Your Favourites added successfully...."
  end

  def about_us

  end

  def contact_us

  end

end