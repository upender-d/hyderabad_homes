class Users::ConfirmationsController < Devise::ConfirmationsController

  def create
    self.resource = resource_class.send_confirmation_instructions(params[resource_name])

    if successfully_sent?(resource)
      #respond_with({}, :location => after_resending_confirmation_instructions_path_for(resource_name))
      redirect_to home_add_to_favourites_path
    else
      respond_with(resource)
    end
  end


  def show
    self.resource = resource_class.confirm_by_token(params[:confirmation_token])
    if resource.errors.empty?
      set_flash_message(:notice, :confirmed) if is_navigational_format?
     sign_in(resource_name, resource)
      #respond_with_navigational(resource){ redirect_to after_confirmation_path_for(resource_name, resource) }
      redirect_to users_dashboard_index_path
    else
      respond_with_navigational(resource.errors, :status => :unprocessable_entity){ render :new }
    end
  end
end